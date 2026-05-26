#include "ael_entry.hpp"

#include <phosphor-logging/lg2.hpp>

#include <format>

namespace amd::ael
{

AelEntry::AelEntry(sdbusplus::bus::bus& /*bus*/, const std::string& path) : _path(path)
{
    lg2::info("AEL: Decorating D-Bus path {PATH} with AEL metadata", "PATH", path);
}

} // namespace amd::ael
