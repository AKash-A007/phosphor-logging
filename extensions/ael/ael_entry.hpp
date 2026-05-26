#pragma once

#include <sdbusplus/bus.hpp>
#include <string>
#include <vector>

namespace amd::ael
{

/** @class AelEntry
 *  @brief Represents an AEL (AMD Event Logging) entry.
 *
 *  Provides the infrastructure for OEM-specific log data representation.
 */
class AelEntry
{
  public:
    AelEntry() = delete;
    AelEntry(const AelEntry&) = delete;
    AelEntry& operator=(const AelEntry&) = delete;
    AelEntry(AelEntry&&) = delete;
    AelEntry& operator=(AelEntry&&) = delete;
    virtual ~AelEntry() = default;

    /** @brief Constructor for AelEntry.
     *
     *  @param[in] bus - The sdbusplus D-Bus bus connection.
     *  @param[in] path - D-Bus object path to decorate.
     */
    AelEntry(sdbusplus::bus::bus& bus, const std::string& path);

  private:
    /** @brief The D-Bus object path this entry decorates. */
    std::string _path;
};

} // namespace amd::ael
