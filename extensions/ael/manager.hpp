#pragma once

#include "ael_entry.hpp"
#include "log_manager.hpp"

#include <sdbusplus/bus.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

namespace amd::ael
{

/**
 * @class Manager
 *
 * The central AEL manager object. Created once at daemon startup and
 * kept alive for the lifetime of phosphor-log-manager.
 *
 * Responsibilities:
 *  - Own the sdbusplus bus reference
 *  - Own the phosphor-log-manager internal::Manager reference
 *  - Own all per-entry AelEntry objects (keyed by log entry ID)
 *  - Delegate create() and erase() to per-entry logic
 *
 * Mirrors: openpower::pels::Manager
 */
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
    /** sdbusplus bus reference — not owned, lives in phosphor-log-manager */
    sdbusplus::bus_t& _bus;

    /** phosphor-log-manager Manager reference */
    phosphor::logging::internal::Manager& _logMgr;

    /**
     * Per-entry AEL objects.
     * Key   : log entry ID
     * Value : AelEntry (owns the D-Bus OEM interface object)
     */
    std::unordered_map<uint32_t, std::unique_ptr<AelEntry>> _entries;
};

} // namespace amd::ael
