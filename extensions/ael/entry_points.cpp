#include "manager.hpp"

#include "extensions.hpp"

namespace amd::ael
{

extern std::unique_ptr<Manager> _manager;

/** @brief Startup function for the AEL extension. */
extern void startup(phosphor::logging::internal::Manager& manager);

/** @brief Hook called on every log entry creation. */
void create(const std::string& msg, uint32_t id, uint64_t timestamp,
            phosphor::logging::Entry::Level /*level*/,
            const phosphor::logging::AdditionalDataArg& additionalData,
            const phosphor::logging::AssociationEndpointsArg& /*assocs*/,
            const phosphor::logging::FFDCArg& /*ffdc*/)
{
    if (_manager)
    {
        // Convert map to vector for Manager signature
        std::vector<std::string> data;
        for (const auto& [k, v] : additionalData)
        {
            data.push_back(k + "=" + v);
        }
        _manager->create(msg, data, id, timestamp);
    }
}

} // namespace amd::ael

// Register the hooks at global scope
using phosphor::logging::Extensions;
REGISTER_EXTENSION_FUNCTION(amd::ael::startup);
REGISTER_EXTENSION_FUNCTION(amd::ael::create);
