#include "manager.hpp"

#include "log_manager.hpp"

#include <phosphor-logging/lg2.hpp>

namespace amd::ael
{

std::unique_ptr<Manager> _manager;

void Manager::create(const std::string& /*errMsg*/,
                    const std::vector<std::string>& /*additionalData*/,
                    uint32_t id, uint64_t /*timestamp*/)
{
    // AEL Decorates the existing BMC Log Entry path
    auto path = std::string("/xyz/openbmc_project/logging/entry/") +
                std::to_string(id);

    _entries[id] = std::make_unique<AelEntry>(_bus, path);
}

/** @brief Startup function for the AEL extension. */
void startup(phosphor::logging::internal::Manager& manager)
{
    lg2::info("AEL: Extension registered and starting up");
    _manager = std::make_unique<Manager>(manager.getBus());
}

} // namespace amd::ael
