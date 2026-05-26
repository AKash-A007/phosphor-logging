#pragma once

#include "ael_entry.hpp"

#include <sdbusplus/bus.hpp>

#include <memory>
#include <map>

namespace phosphor::logging::internal
{
class Manager;
}

namespace amd::ael
{

/** @class Manager
 *  @brief Manages the AEL extension state and log entry creation.
 *
 *  The Manager is responsible for hooking into the phosphor-logging
 *  event creation flow and generating AEL-specific metadata.
 */
class Manager
{
  public:
    Manager() = delete;
    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;
    Manager(Manager&&) = delete;
    Manager& operator=(Manager&&) = delete;
    virtual ~Manager() = default;

    /** @brief Constructor for the Manager.
     *
     *  @param[in] bus - The sdbusplus D-Bus bus connection.
     */
    explicit Manager(sdbusplus::bus::bus& bus) : _bus(bus) {}

    /** @brief Main entry point for the AEL extension to create an entry.
     *
     *  This function is called by the phosphor-log-manager when a new
     *  standard event is created.
     *
     *  @param[in] errMsg - The error message/event ID (e.g. "xyz.openbmc_project.Fan.Error.FanFailed")
     *  @param[in] additionalData - The metadata associated with the event.
     *  @param[in] id - The unique numeric ID of the log entry.
     *  @param[in] timestamp - The creation timestamp.
     */
    void create(const std::string& errMsg,
                const std::vector<std::string>& additionalData, uint32_t id,
                uint64_t timestamp);

  private:
    /** @brief Reference to the D-Bus bus. */
    sdbusplus::bus::bus& _bus;

    /** @brief Collection of AEL decorators maintained by the manager.
     *  Maps Entry ID to the AEL metadata implementation.
     */
    std::map<uint32_t, std::unique_ptr<AelEntry>> _entries;
};

/** @brief Startup function for the AEL extension.
 *
 *  Registered via REGISTER_EXTENSION_FUNCTION.
 *
 *  @param[in] manager - The phosphor-logging internal manager.
 */
void startup(phosphor::logging::internal::Manager& manager);

} // namespace amd::ael
