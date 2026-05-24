// SPDX-License-Identifier: Apache-2.0
// extensions/ael/entry_points.cpp

#include "extensions.hpp"
#include "log_manager.hpp"
#include "manager.hpp"

#include <phosphor-logging/lg2.hpp>

#include <memory>

namespace amd::ael
{

using namespace phosphor::logging;

// Global Manager instance
std::unique_ptr<Manager> globalManager;

/**
 * @brief Startup function — called once by phosphor-log-manager.
 */
void aelStartup(phosphor::logging::internal::Manager& logManager)
{
    lg2::info("AEL extension starting up");

    globalManager = std::make_unique<Manager>(
        logManager.getBus(),
        logManager);

    lg2::info("AEL extension ready");
}

/**
 * @brief Create callback — called for every new log entry.
 */
void aelCreate(const std::string& message, uint32_t id, uint64_t /*timestamp*/,
               phosphor::logging::Entry::Level severity,
               const phosphor::logging::AdditionalDataArg& additionalData,
               const phosphor::logging::AssociationEndpointsArg& /*assocs*/,
               const phosphor::logging::FFDCArg& ffdc)
{
    if (globalManager)
    {
        globalManager->create(message, severity, additionalData, ffdc, id);
    }
}

/**
 * @brief Erase callback — called before a log entry is deleted.
 */
void aelErase(uint32_t id)
{
    if (globalManager)
    {
        globalManager->erase(id);
    }
}

// Extension registration
REGISTER_EXTENSION_FUNCTION(aelStartup);
REGISTER_EXTENSION_FUNCTION(aelCreate);
REGISTER_EXTENSION_FUNCTION(aelErase);

} // namespace amd::ael