#include "manager.hpp"

#include <phosphor-logging/lg2.hpp>

namespace amd::ael
{

Manager::Manager(sdbusplus::bus_t& bus,
                 phosphor::logging::internal::Manager& logMgr) :
    _bus(bus),
    _logMgr(logMgr)
{
    // Implementation of startup logic if any.
    // In PEL, this might involve loading existing logs from disk,
    // but AEL is D-Bus only and follows phosphor-log-manager's entries.
}

void Manager::create(const std::string& message,
                     phosphor::logging::Entry::Level /*severity*/,
                     const phosphor::logging::AdditionalDataArg& additionalData,
                     const phosphor::logging::FFDCArg& /*ffdc*/, uint32_t id)
{
    if (_entries.find(id) != _entries.end())
    {
        lg2::warning("AEL: entry {ID} already exists in manager", "ID", id);
        return;
    }

    // Create a new AelEntry. 
    // The AelEntry constructor will handle attaching the OEM D-Bus interface.
    _entries[id] = std::make_unique<AelEntry>(id, message, additionalData, _bus);
}

void Manager::erase(uint32_t id)
{
    auto it = _entries.find(id);
    if (it != _entries.end())
    {
        // Removing from map destroys the unique_ptr, which destroys AelEntry,
        // which removes the D-Bus interface.
        _entries.erase(it);
    }
}

} // namespace amd::ael