// SPDX-License-Identifier: Apache-2.0
// extensions/ael/ael.hpp
//
// AEL (AMD Event Logging) phosphor-logging extension.
//
// Registers two lifecycle callbacks with phosphor-log-manager:
//   create() — called when a log entry is created
//   erase()  — called when a log entry is deleted
//
// The create() hook attaches xyz.openbmc_project.Logging.Entry.Oem
// to the existing log entry D-Bus object and populates:
//   ServiceCode       → AFID looked up from build-time generated map
//   OemAdditionalData → metadata extracted from lg2 additionalData

#pragma once

#include "log_manager.hpp"
#include "extensions.hpp"

#include <phosphor-logging/lg2.hpp>

#include <cstdint>
#include <map>
#include <string>

namespace amd::ael
{

/**
 * @brief Called synchronously when a new log entry is created.
 *
 * Attaches xyz.openbmc_project.Logging.Entry.Oem to the log entry
 * and populates ServiceCode + OemAdditionalData inline.
 * No async calls. No D-Bus signal listening.
 */
void create(const std::string& message, uint32_t id, uint64_t timestamp,
            phosphor::logging::Entry::Level severity,
            const phosphor::logging::AdditionalDataArg& additionalData,
            const phosphor::logging::AssociationEndpointsArg& associations,
            const phosphor::logging::FFDCArg& ffdc);

/**
 * @brief Called before a log entry is deleted.
 * Removes the OEM interface object for this entry ID.
 */
void erase(uint32_t id);

/**
 * @brief Exposed for unit testing — extract a key from AdditionalData.
 * AdditionalData is a vector of "KEY=VALUE" strings.
 */
std::string extractAdditionalDataValue(
    const phosphor::logging::AdditionalDataArg& additionalData,
    const std::string& key);

/**
 * @brief Exposed for unit testing — build the OemAdditionalData map.
 */
std::map<std::string, std::string> buildOemData(
    const std::string& message,
    const phosphor::logging::AdditionalDataArg& additionalData);

} // namespace amd::ael