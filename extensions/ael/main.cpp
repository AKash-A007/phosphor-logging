#include "dbus_exporter.hpp"
#include "model.hpp"
#include "serializer.hpp"
#include "validator.hpp"

#include <sdbusplus/bus.hpp>

#include <memory>

using namespace phosphor::logging::ael;

int main()
{
    auto bus = sdbusplus::bus::new_default();

    AelSimple obj;
    obj.afid = "0x1001";
    obj.fruList = {"/xyz/.../dimm0"};

    if (!AelValidator::validate(obj))
        return -1;

    auto map = AelSerializer::toMap(obj);

    AelDbusExporter exporter(bus);
    exporter.attach("/xyz/openbmc_project/logging/entry/1", map);

    while (true)
    {
        bus.process_discard();
        bus.wait();
    }
}
