#pragma once

#include "ael_entry.hpp"
#include "log_manager.hpp"

#include <sdbusplus/bus.hpp>

#include <nlohmann/json.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace amd::ael
{

class Manager
{
  public:
    Manager() = delete;
    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;
    Manager(Manager&&) = delete;
    Manager& operator=(Manager&&) = delete;

    /**
     * @brief Constructor.
     *
     * @param[in] bus     - sdbusplus bus connection (owned by log-manager)
     * @param[in] logMgr  - reference to phosphor-log-manager's Manager
     */
    Manager(sdbusplus::bus_t& bus,
            phosphor::logging::internal::Manager& logMgr);

    ~Manager() = default;

    /**
     * @brief Called when a new log entry is created.
     *
     * Creates an AelEntry for this log ID, attaches
     * xyz.openbmc_project.Logging.Entry.Oem to the D-Bus object,
     * and populates ServiceCode + OemAdditionalData.
     *
     * @param[in] message        - OpenBMC event ID string
     * @param[in] severity       - log severity level
     * @param[in] additionalData - KEY=VALUE metadata from lg2::commit()
     * @param[in] ffdc           - FFDC file descriptors
     * @param[in] id             - assigned log entry ID
     */
    void create(const std::string& message,
                phosphor::logging::Entry::Level severity,
                const phosphor::logging::AdditionalDataArg& additionalData,
                const phosphor::logging::FFDCArg& ffdc,
                uint32_t id);

    /**
     * @brief Called before a log entry is deleted.
     * Removes the AelEntry for this ID — D-Bus interface goes with it.
     *
     * @param[in] id - log entry ID being deleted
     */
    void erase(uint32_t id);

  private:
    /**
     * @brief Load the AEL configuration from JSON.
     */
    void loadConfig();

    /**
     * @brief Resolve the origin of condition for a log entry.
     */
    std::string resolveOrigin(
        uint32_t id,
        const phosphor::logging::AdditionalDataArg& additionalData);

    /**
     * @brief Resolve a FRU hint into an inventory path using the FRUMap.
     */
    std::string resolveFruHint(const std::string& hint);

    /**
     * @brief Expand associations based on AFID and origin path.
     */
    std::vector<std::tuple<std::string, std::string, std::string>>
        expandAssociations(const std::string& afid, const std::string& origin);

    /**
     * @brief Check if a match rule pattern matches a property value.
     */
    bool checkMatch(const nlohmann::json& matchBlock,
                    const std::string& propertyValue);

    /**
     * @brief Resolve a property value (e.g., SensorName) from additionalData.
     */
    std::string resolveProperty(
        const std::string& propertyName,
        const phosphor::logging::AdditionalDataArg& additionalData);

    /** sdbusplus bus reference — not owned, lives in phosphor-log-manager */
    sdbusplus::bus_t& _bus;

    /** phosphor-log-manager Manager reference */
    phosphor::logging::internal::Manager& _logMgr;

    /** AEL configuration loaded from JSON */
    nlohmann::json _config;

    /**
     * Per-entry AEL objects.
     * Key   : log entry ID
     * Value : AelEntry (owns the D-Bus OEM interface object)
     */
    std::unordered_map<uint32_t, std::unique_ptr<AelEntry>> _entries;
};

} // namespace amd::ael
