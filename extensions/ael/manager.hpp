#pragma once

#include "dbus_exporter.hpp"
#include "extensions.hpp"

#include <sdbusplus/bus.hpp>

#include <string>

namespace phosphor::logging::internal
{
class Manager;
}

namespace phosphor::logging::extensions::ael
{

/**
 * @brief AEL extension manager
 *
 * Handles lifecycle and integration into phosphor-logging
 */
class AelManager
{
  public:
    explicit AelManager(sdbusplus::bus_t& bus);

    AelManager(const AelManager&) = delete;
    AelManager& operator=(const AelManager&) = delete;
    AelManager(AelManager&&) = delete;
    AelManager& operator=(AelManager&&) = delete;

    ~AelManager() = default;

    void onCreate(const std::string& message,
                  const phosphor::logging::AdditionalDataArg& additionalData,
                  uint32_t id, uint64_t timestamp);

  private:
    /** @brief Load AEL registry from JSON */
    void loadRegistry();

    sdbusplus::bus_t& bus;

    /** @brief Persistent D-Bus exporter to keep interfaces alive */
    AelDbusExporter exporter;

    /** @brief Mapping from OpenBMC Message ID to AFID */
    std::map<std::string, std::string> _registry;
};

void startup(phosphor::logging::internal::Manager& manager);

} // namespace phosphor::logging::extensions::ael
