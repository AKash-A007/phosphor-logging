#pragma once

#include <xyz/openbmc_project/Logging/Entry/Oem/server.hpp>

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>

#include <map>
#include <memory>
#include <string>

namespace phosphor::logging::extensions::ael
{

namespace details
{
using OemInterface = sdbusplus::server::object_t<
    sdbusplus::xyz::openbmc_project::Logging::Entry::server::Oem>;
}

/**
 * @brief Attaches OEM metadata interface to an existing Logging.Entry path.
 *
 * Uses generated sdbusplus bindings (OemInterface) to register
 * OemAdditionalData as a property.
 */
class AelDbusExporter
{
  public:
    explicit AelDbusExporter(sdbusplus::bus_t& bus) : _bus(bus) {}

    /**
     * @brief Attach OEM metadata interface to the given path.
     */
    void attach(const std::string& path,
                const std::map<std::string, std::string>& data)
    {
        if (data.empty() || _entries.contains(path))
        {
            return;
        }

        auto entry = std::make_unique<details::OemInterface>(_bus, path.c_str());
        entry->oemAdditionalData(data);

        _entries.emplace(path, std::move(entry));
    }

    /**
     * @brief Remove the OEM interface from the given path.
     */
    void remove(const std::string& path)
    {
        _entries.erase(path);
    }

  private:
    sdbusplus::bus_t& _bus;

    /** @brief Keeps the interfaces alive. */
    std::map<std::string, std::unique_ptr<details::OemInterface>> _entries;
};

} // namespace phosphor::logging::extensions::ael
