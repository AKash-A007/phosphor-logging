#include "extensions.hpp"
#include "manager.hpp"

namespace phosphor::logging::extensions::ael
{

extern std::unique_ptr<AelManager> managerInstance;

void startup(phosphor::logging::internal::Manager& manager)
{
    managerInstance = std::make_unique<AelManager>(manager.getBus());
}

void create(const std::string& message, uint32_t id, uint64_t timestamp,
            phosphor::logging::Entry::Level,
            const phosphor::logging::AdditionalDataArg& additionalData,
            const phosphor::logging::AssociationEndpointsArg&,
            const phosphor::logging::FFDCArg&)
{
    if (managerInstance)
    {
        managerInstance->onCreate(message, additionalData, id, timestamp);
    }
}

} // namespace phosphor::logging::extensions::ael

using phosphor::logging::Extensions;
REGISTER_EXTENSION_FUNCTION(phosphor::logging::extensions::ael::startup);

REGISTER_EXTENSION_FUNCTION(phosphor::logging::extensions::ael::create);
