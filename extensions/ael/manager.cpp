#include "manager.hpp"

#include "dbus_exporter.hpp"
#include "log_manager.hpp"
#include "model.hpp"
#include "serializer.hpp"
#include "validator.hpp"

#include <nlohmann/json.hpp>
#include <phosphor-logging/lg2.hpp>

#include <fstream>

namespace phosphor::logging::extensions::ael
{

std::unique_ptr<AelManager> managerInstance;

AelManager::AelManager(sdbusplus::bus_t& bus) : bus(bus), exporter(bus)
{
    loadRegistry();
}

void AelManager::loadRegistry()
{
    const char* registryPath = "/usr/share/phosphor-logging/ael/ael-registry.json";
    const char* localPath = "./ael-registry.json";

    std::string path =
        std::filesystem::exists(registryPath) ? registryPath : localPath;

    if (!std::filesystem::exists(path))
    {
        lg2::warning("AEL: registry not found at {PATH}", "PATH", path);
        return;
    }

    try
    {
        std::ifstream file(path);
        auto data = nlohmann::json::parse(file);

        if (data.contains("mappings"))
        {
            for (auto it = data["mappings"].begin(); it != data["mappings"].end();
                 ++it)
            {
                _registry.emplace(it.key(), it.value()["afid"]);
            }
        }
        lg2::info("AEL: loaded {COUNT} mappings from registry", "COUNT",
                  _registry.size());
    }
    catch (const std::exception& e)
    {
        lg2::error("AEL: failed to parse registry: {ERROR}", "ERROR", e.what());
    }
}

void AelManager::onCreate(
    const std::string& message,
    const phosphor::logging::AdditionalDataArg& /*additionalData*/, uint32_t id,
    uint64_t /*timestamp*/)
{
    const auto path = std::format("/xyz/openbmc_project/logging/entry/{}", id);

    lg2::debug("AEL: processing log entry path={PATH} msg={MSG}", "PATH", path,
              "MSG", message);

    auto it = _registry.find(message);
    if (it == _registry.end())
    {
        return;
    }

    AelSimple aelObject;
    aelObject.afid = it->second;

    if (!AelValidator::validate(aelObject))
    {
        lg2::warning("AEL: validation failed for entry {ID} afid={AFID}", "ID",
                     id, "AFID", aelObject.afid);
        return;
    }

    const auto serializedData = AelSerializer::toMap(aelObject);

    exporter.attach(path, serializedData);
}

void startup(phosphor::logging::internal::Manager& manager)
{
    lg2::info("AEL extension initialized");

    managerInstance = std::make_unique<AelManager>(manager.getBus());
}

} // namespace phosphor::logging::extensions::ael
