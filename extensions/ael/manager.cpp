#include "manager.hpp"

#include "dbus_exporter.hpp"
#include "log_manager.hpp"
#include "model.hpp"
#include "serializer.hpp"
#include "validator.hpp"

#include <phosphor-logging/lg2.hpp>

namespace phosphor::logging::extensions::ael
{

std::unique_ptr<AelManager> managerInstance;

void AelManager::onCreate(
    const std::string& /*message*/,
    const phosphor::logging::AdditionalDataArg& /*additionalData*/, uint32_t id,
    uint64_t /*timestamp*/)
{
    const auto path = std::format("/xyz/openbmc_project/logging/entry/{}", id);

    lg2::debug("AEL: processing log entry path={PATH}", "PATH", path);

    /*
     * Temporary placeholder logic:
     * In real implementation, plugins / rules will populate AEL metadata
     */
    AelSimple aelObject;
    aelObject.afid = "0x1001";

    if (!AelValidator::validate(aelObject))
    {
        lg2::warning("AEL: validation failed for entry {ID}", "ID", id);
        return;
    }

    const auto serializedData = AelSerializer::toMap(aelObject);

    exporter.attach(path, serializedData);
}

void startup(phosphor::logging::internal::Manager& manager)
{
    lg2::info("AEL extension initialized");

    managerInstance = std::make_unique<AelManager>(manager.getBus());
}

} // namespace phosphor::logging::extensions::ael
