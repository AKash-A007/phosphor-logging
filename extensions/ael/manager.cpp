#include "manager.hpp"
#include "event_afid_map.hpp"

#include <phosphor-logging/lg2.hpp>

#include <fstream>
#include <regex>
#include <vector>

namespace amd::ael
{

Manager::Manager(sdbusplus::bus_t& bus, phosphor::logging::internal::Manager& logMgr) :
    _bus(bus), _logMgr(logMgr)
{
    loadConfig();
}

void Manager::loadConfig()
{
    const char* configPath = "/usr/share/phosphor-logging/ael_config.json";
    
    if (!std::fstream(configPath).good())
    {
        configPath = "extensions/ael/ael_config.json";
    }

    try
    {
        std::ifstream file(configPath);
        if (file.is_open())
        {
            file >> _config;
            lg2::info("AEL: Loaded configuration from {PATH}", "PATH", configPath);
        }
        else
        {
            lg2::error("AEL: Failed to open config file {PATH}", "PATH",
                       configPath);
        }
    }
    catch (const std::exception& e)
    {
        lg2::error("AEL: Error parsing config: {ERROR}", "ERROR", e.what());
    }
}

void Manager::create(const std::string& message,
                phosphor::logging::Entry::Level severity,
                const phosphor::logging::AdditionalDataArg& additionalData,
                [[maybe_unused]] const phosphor::logging::FFDCArg& ffdc,
                uint32_t id)
{
    std::string afid = "AFID_UNKNOWN";
    int highestPriority = -1;

    if (_config.contains("BMCEventMatchRules"))
    {
        for (const auto& rule : _config["BMCEventMatchRules"])
        {
            if (rule["Event"] == message)
            {
                bool fullMatch = true;
                if (rule.contains("Match"))
                {
                    for (auto it = rule["Match"].begin(); it != rule["Match"].end();
                         ++it)
                    {
                        std::string propName = it.key();
                        std::string propVal =
                            resolveProperty(propName, additionalData);

                        if (!checkMatch(it.value(), propVal))
                        {
                            fullMatch = false;
                            break;
                        }
                    }
                }

                if (fullMatch)
                {
                    int priority = rule.value("Priority", 0);
                    if (priority > highestPriority)
                    {
                        afid = "AFID_" + rule["AFID"].get<std::string>();
                        highestPriority = priority;
                    }
                }
            }
        }
    }

    if (afid == "AFID_UNKNOWN")
    {
        afid = lookupAfid(message);
    }
    
    if (afid == "AFID_UNKNOWN")
    {
        afid = "AFID_DEFAULT_UNKNOWN";
    }

    lg2::info("AEL: Creating entry {ID} for {EVENT} (AFID: {AFID})", "ID", id,
              "EVENT", message, "AFID", afid);

    auto origin = resolveOrigin(id, additionalData);
    if (!origin.empty())
    {
        lg2::info("AEL: Resolved origin {PATH} for entry {ID}", "PATH", origin,
                  "ID", id);
        
        auto associations = expandAssociations(afid, origin);
        
        auto it = _logMgr.entries.find(id);
        if (it != _logMgr.entries.end())
        {
            auto currentAssocs = it->second->associations();
            for (const auto& [fwd, rev, path] : associations)
            {
                currentAssocs.push_back({fwd, rev, path});
            }
            it->second->associations(currentAssocs);
        }
    }

    auto entry = std::make_unique<AelEntry>(_bus, id, message, severity,
                                            additionalData, afid);
    _entries[id] = std::move(entry);
}

std::string Manager::resolveOrigin(uint32_t id,
                                   const phosphor::logging::AdditionalDataArg& additionalData)
{
    auto fruList = AelEntry::extractValue(additionalData, "AEL.FRU_LIST");
    if (!fruList.empty())
    {
        return fruList.substr(0, fruList.find(','));
    }

    auto it = _logMgr.entries.find(id);
    if (it != _logMgr.entries.end())
    {
        for (const auto& [fwd, rev, path] : it->second->associations())
        {
            if (path.find("/inventory/") != std::string::npos)
            {
                return path;
            }
        }
    }

    auto fruHint = AelEntry::extractValue(additionalData, "FRU");
    if (!fruHint.empty())
    {
        auto resolved = resolveFruHint(fruHint);
        if (!resolved.empty()) return resolved;
    }

    return "/xyz/openbmc_project/inventory/system/chassis";
}

std::string Manager::resolveFruHint(const std::string& hint)
{
    if (!_config.contains("FRUMap")) return "";

    auto fruMap = _config["FRUMap"];

    // Exact match
    if (fruMap.contains("Exact") && fruMap["Exact"].contains(hint))
    {
        return fruMap["Exact"][hint];
    }

    // Pattern match
    if (fruMap.contains("Pattern"))
    {
        for (const auto& p : fruMap["Pattern"])
        {
            try {
                std::regex re(p["Match"].get<std::string>());
                std::smatch match;
                if (std::regex_match(hint, match, re))
                {
                    std::string templ = p["Template"].get<std::string>();
                    size_t pos = templ.find("{0}");
                    if (pos != std::string::npos && match.size() > 1)
                    {
                        templ.replace(pos, 3, match[1].str());
                    }
                    return templ;
                }
            } catch (...) {}
        }
    }

    return "";
}

std::vector<std::tuple<std::string, std::string, std::string>>
Manager::expandAssociations(const std::string& afid, const std::string& origin)
{
    std::vector<std::tuple<std::string, std::string, std::string>> assocs;
    
    // Always add the base "origin" association
    assocs.emplace_back("origin", "log_entry", origin);

    if (_config.contains("AFIDRules"))
    {
        for (const auto& rule : _config["AFIDRules"])
        {
            std::string ruleAfid = "AFID_" + rule["AFID"].get<std::string>();
            if (ruleAfid == afid)
            {
                for (const auto& type : rule["Expand"])
                {
                    if (type == "powered_by")
                        assocs.emplace_back("powered_by", "powers", origin);
                    else if (type == "cooled_by")
                        assocs.emplace_back("cooled_by", "cools", origin);
                }
            }
        }
    }

    return assocs;
}

bool Manager::checkMatch(const nlohmann::json& matchBlock,
                        const std::string& propertyValue)
{
    if (matchBlock.value("Optional", false) && propertyValue.empty())
    {
        return true;
    }

    if (matchBlock.contains("Pattern"))
    {
        try
        {
            std::regex re(matchBlock["Pattern"].get<std::string>());
            return std::regex_match(propertyValue, re);
        }
        catch (...)
        {
            return false;
        }
    }

    if (matchBlock.contains("Equals"))
    {
        return propertyValue == matchBlock["Equals"].get<std::string>();
    }

    return true; // No constraints defined for this field
}

std::string Manager::resolveProperty(
    const std::string& propertyName,
    const phosphor::logging::AdditionalDataArg& additionalData)
{
    auto val = AelEntry::extractValue(additionalData, propertyName);
    if (!val.empty()) return val;

    // Map common aliases
    if (propertyName == "SensorName")
        return AelEntry::extractValue(additionalData, "SENSOR_NAME");
    if (propertyName == "Unit")
        return AelEntry::extractValue(additionalData, "UNIT");

    return "";
}

void Manager::erase(uint32_t id)
{
    auto it = _entries.find(id);
    if (it != _entries.end())
    {
        _entries.erase(it);
    }
}

} // namespace amd::ael