/**
 * @file dbus_exporter.hpp
 * @brief DBus interface exporter for AEL OEM metadata
 */

#pragma once

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/interface.hpp>

#include <map>
#include <memory>
#include <string>

namespace phosphor::logging::extensions::ael
{

namespace details
{
/** @brief DBus interface name for OEM extension */
constexpr const char* oemInterface = "xyz.openbmc_project.Logging.Entry.Oem";

/** @brief Property name for OEM metadata */
constexpr const char* oemProperty = "OemAdditionalData";
} // namespace details

/**
 * @brief Attaches OEM metadata to Logging.Entry DBus object
 *
 * This class exposes AEL metadata via:
 * xyz.openbmc_project.Logging.Entry.Oem
 *
 * Responsibilities:
 *  - Attach interface to existing log entry object
 *  - Register OemAdditionalData property
 *  - Maintain interface lifetime
 *
 * Notes:
 *  - Interface is attached only once per object path
 *  - Property is read-only
 */
class AelDbusExporter
{
  public:
    /**
     * @brief Constructor
     * @param bus DBus bus reference
     */
    explicit AelDbusExporter(sdbusplus::bus_t& bus) : bus(bus) {}

    /**
     * @brief Attach OEM metadata interface
     *
     * @param path DBus object path (log entry)
     * @param data Serialized AEL metadata
     */
    void attach(const std::string& path,
                const std::map<std::string, std::string>& data)
    {
        if (data.empty())
        {
            return;
        }

        // Prevent duplicate interface creation
        if (interfaces.find(path) != interfaces.end())
        {
            return;
        }

        auto intf = bus.add_interface(path, details::oemInterface);

        intf->register_property(details::oemProperty, data,
                                sdbusplus::vtable::property_::readOnly);

        intf->initialize();

        // Important: keep interface alive
        interfaces.emplace(path, std::move(intf));
    }

    /**
     * @brief Remove interface (optional cleanup)
     *
     * @param path DBus object path
     */
    void remove(const std::string& path)
    {
        interfaces.erase(path);
    }

  private:
    /** @brief DBus bus reference */
    sdbusplus::bus_t& bus;

    /**
     * @brief Interface storage for lifetime management
     *
     * Ensures that dynamically created interfaces are not destroyed.
     */
    std::map<std::string, std::shared_ptr<sdbusplus::server::interface_t>>
        interfaces;
};

} // namespace phosphor::logging::extensions::ael
