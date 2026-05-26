#include "manager.hpp"

#include <phosphor-logging/lg2.hpp>

namespace phosphor::logging::extensions::ael
{

std::unique_ptr<AelManager> managerInstance;

void AelManager::onCreate(
    const std::string& message,
    const phosphor::logging::AdditionalDataArg& /*additionalData*/, uint32_t id,
    uint64_t /*timestamp*/)
{
    auto path = std::format("/xyz/openbmc_project/logging/entry/{}", id);

    lg2::debug("AEL: entry created path={PATH} message={MESSAGE}", "PATH", path,
               "MESSAGE", message);

    // Future: attach AEL metadata here (commit 2)
}

void startup(phosphor::logging::internal::Manager& manager)
{
    lg2::info("AEL extension initialized");

    managerInstance = std::make_unique<AelManager>(manager.getBus());
}

} // namespace phosphor::logging::extensions::ael
