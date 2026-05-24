// extensions/ael/ael_entry.hpp

#pragma once

#include "extensions.hpp"
#include "log_manager.hpp"

// Generated from yaml/xyz/openbmc_project/Logging/Entry/Oem.interface.yaml
#include <xyz/openbmc_project/Logging/Entry/Oem/server.hpp>

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>

#include <cstdint>
#include <map>
#include <memory>
#include <string>

namespace amd::ael
{

using OemEntryIface = sdbusplus::server::object_t<
    sdbusplus::xyz::openbmc_project::Logging::Entry::server::Oem>;

/**
 * @class AelEntry
 * Lifecycle:
 *   Created in Manager::create()  → attaches OEM interface to D-Bus
 *   Destroyed in Manager::erase() → removes OEM interface from D-Bus
 *
 */
class AelEntry
{
  public:
    AelEntry() = delete;
    AelEntry(const AelEntry&) = delete;
    AelEntry& operator=(const AelEntry&) = delete;

    /**
     * @brief Constructor — attaches Entry.Oem and fills properties.
     *
     * @param[in] id             - log entry ID
     * @param[in] message        - OpenBMC event ID
     * @param[in] additionalData - KEY=VALUE metadata from lg2::commit()
     * @param[in] bus            - sdbusplus bus
     */
    AelEntry(sdbusplus::bus_t& bus,
             uint32_t id,
             const std::string& message,
             phosphor::logging::Entry::Level severity,
             const phosphor::logging::AdditionalDataArg& additionalData,
             const std::string& serviceCode);

    ~AelEntry() = default;

    /** @brief The AFID/ServiceCode for this entry */
    const std::string& serviceCode() const { return _serviceCode; }

    /** @brief The OemAdditionalData map for this entry */
    const std::map<std::string, std::string>& oemData() const
    {
        return _oemData;
    }

    /**
     * @brief Extract a value from AdditionalData by key.
     * AdditionalData items are "KEY=VALUE" strings.
     */
    static std::string extractValue(
        const phosphor::logging::AdditionalDataArg& additionalData,
        const std::string& key);

    /**
     * @brief Build the OemAdditionalData map from the additionalData.
     * Maps known AEL keys to camelCase Redfish-friendly names.
     */
    static std::map<std::string, std::string> buildOemData(
        const std::string& message,
        const phosphor::logging::AdditionalDataArg& additionalData);

  private:

    /** Log entry ID */
    uint32_t _id;

    /** AFID looked up from build-time generated map */
    std::string _serviceCode;

    /** OEM metadata map */
    std::map<std::string, std::string> _oemData;

    /**
     * The sdbusplus OEM interface object.
     * Attached to /xyz/openbmc_project/logging/entry/<id>.
     * Destroyed when AelEntry is destroyed → interface removed from D-Bus.
     */
    std::unique_ptr<OemEntryIface> _iface;
};

} // namespace amd::ael