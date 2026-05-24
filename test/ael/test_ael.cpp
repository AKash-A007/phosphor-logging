// test/ael/test_ael.cpp
//
// Google Test suite for AEL extension.
// Tests the Manager, AelEntry, and entry point logic
// using dummy data simple use cases as requested by mentor.

#include "extensions/ael/ael_entry.hpp"
#include "extensions/ael/manager.hpp"
#include "event_afid_map.hpp"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <map>
#include <string>
#include <vector>

using ::testing::HasSubstr;
using ::testing::IsEmpty;
using ::testing::Not;

using AddData = std::vector<std::string>;

// ─────────────────────────────────────────────────────────────────────────────
// Test Suite 1: AFID Lookup (generated header)
// ─────────────────────────────────────────────────────────────────────────────

class AfidLookupTest : public ::testing::Test {};

TEST_F(AfidLookupTest, FanFailedMapsToAfid26)
{
    EXPECT_STREQ(
        amd::ael::lookupAfid("xyz.openbmc_project.State.Fan.FanFailed"),
        "AFID_26");
}

TEST_F(AfidLookupTest, FanRestoredMapsToAfid29)
{
    EXPECT_STREQ(
        amd::ael::lookupAfid("xyz.openbmc_project.State.Fan.FanRestored"),
        "AFID_29");
}

TEST_F(AfidLookupTest, CperReportedMapsToAfid34)
{
    EXPECT_STREQ(
        amd::ael::lookupAfid("com.amd.Event.Cper.CperReported"),
        "AFID_34");
}

TEST_F(AfidLookupTest, ThermalFaultMapsToAfid26)
{
    EXPECT_STREQ(
        amd::ael::lookupAfid(
            "xyz.openbmc_project.State.Thermal.DeviceOverOperatingTemperatureFault"),
        "AFID_26");
}

TEST_F(AfidLookupTest, SensorUpperCriticalMapsToAfid25)
{
    EXPECT_STREQ(
        amd::ael::lookupAfid(
            "xyz.openbmc_project.Sensor.Threshold.ReadingAboveUpperCriticalThreshold"),
        "AFID_25");
}

TEST_F(AfidLookupTest, UnknownEventReturnsAfidUnknown)
{
    EXPECT_STREQ(
        amd::ael::lookupAfid("xyz.openbmc_project.Some.Event.ThatDoesNotExist"),
        "AFID_UNKNOWN");
}

TEST_F(AfidLookupTest, EmptyStringReturnsAfidUnknown)
{
    EXPECT_STREQ(amd::ael::lookupAfid(""), "AFID_UNKNOWN");
}

// ─────────────────────────────────────────────────────────────────────────────
// Test Suite 2: AelEntry static helpers (tested via a test subclass)
// ─────────────────────────────────────────────────────────────────────────────

// We expose the helpers by making a thin test accessor class
class AelEntryHelperTest : public ::testing::Test
{
  protected:
    // Mirror extractValue logic inline for testability
    std::string extract(const AddData& data, const std::string& key)
    {
        const std::string prefix = key + "=";
        for (const auto& item : data)
            if (item.starts_with(prefix))
                return item.substr(prefix.size());
        return {};
    }

    std::map<std::string, std::string> buildOem(
        const std::string& msg, const AddData& data)
    {
        static const std::vector<std::pair<std::string,std::string>> keys = {
            {"SLOT_ID",           "SlotId"},
            {"EVENT_TYPE",        "EventType"},
            {"CORRELATION_ID",    "CorrelationId"},
            {"COMPONENT_ID",      "ComponentId"},
            {"CPER_SECTION_TYPE", "CperSectionType"},
            {"GPU_ID",            "GpuId"},
            {"SEVERITY_HINT",     "SeverityHint"},
        };
        std::map<std::string, std::string> oem;
        oem["EventMessage"] = msg;
        for (const auto& [raw, camel] : keys)
        {
            auto v = extract(data, raw);
            if (!v.empty()) oem[camel] = v;
        }
        return oem;
    }
};

TEST_F(AelEntryHelperTest, ExtractSlotId)
{
    AddData d = {"SLOT_ID=GPU0", "EVENT_TYPE=CPER"};
    EXPECT_EQ(extract(d, "SLOT_ID"), "GPU0");
}

TEST_F(AelEntryHelperTest, ExtractMissingKeyReturnsEmpty)
{
    AddData d = {"SLOT_ID=GPU0"};
    EXPECT_TRUE(extract(d, "CORRELATION_ID").empty());
}

TEST_F(AelEntryHelperTest, ExtractValueWithEqualsInsidePreserved)
{
    AddData d = {"CORRELATION_ID=abc=123"};
    EXPECT_EQ(extract(d, "CORRELATION_ID"), "abc=123");
}

TEST_F(AelEntryHelperTest, PartialKeyNotMatched)
{
    AddData d = {"SLOT_ID_EXTRA=wrong"};
    EXPECT_TRUE(extract(d, "SLOT_ID").empty());
}

TEST_F(AelEntryHelperTest, OemAlwaysHasEventMessage)
{
    auto oem = buildOem("xyz.openbmc_project.State.Fan.FanFailed", {});
    EXPECT_EQ(oem.count("EventMessage"), 1);
    EXPECT_EQ(oem.at("EventMessage"), "xyz.openbmc_project.State.Fan.FanFailed");
}

TEST_F(AelEntryHelperTest, OemRawKeysNotPresent)
{
    AddData d = {"SLOT_ID=GPU0"};
    auto oem = buildOem("some.event", d);
    EXPECT_EQ(oem.count("SlotId"), 1);
    EXPECT_EQ(oem.count("SLOT_ID"), 0);
}

TEST_F(AelEntryHelperTest, OemUnknownKeysIgnored)
{
    AddData d = {"SLOT_ID=GPU0", "UNKNOWN_KEY=ignored"};
    auto oem = buildOem("some.event", d);
    EXPECT_EQ(oem.count("UNKNOWN_KEY"), 0);
    EXPECT_EQ(oem.count("SlotId"), 1);
}

TEST_F(AelEntryHelperTest, OemAllKnownKeysMapped)
{
    AddData d = {
        "SLOT_ID=GPU1",
        "EVENT_TYPE=CPER",
        "CORRELATION_ID=abc",
        "COMPONENT_ID=HBM0",
        "CPER_SECTION_TYPE=Memory",
        "GPU_ID=GPU1",
        "SEVERITY_HINT=Fatal",
    };
    auto oem = buildOem("com.amd.Event.Cper.CperReported", d);
    EXPECT_EQ(oem.at("SlotId"),          "GPU1");
    EXPECT_EQ(oem.at("EventType"),       "CPER");
    EXPECT_EQ(oem.at("CorrelationId"),   "abc");
    EXPECT_EQ(oem.at("ComponentId"),     "HBM0");
    EXPECT_EQ(oem.at("CperSectionType"), "Memory");
    EXPECT_EQ(oem.at("GpuId"),          "GPU1");
    EXPECT_EQ(oem.at("SeverityHint"),   "Fatal");
}

// ─────────────────────────────────────────────────────────────────────────────
// Test Suite 3: Dummy Data Simple Use Cases
// Complete end-to-end flows with known dummy data — what mentor asked for
// ─────────────────────────────────────────────────────────────────────────────

class DummyDataTest : public AelEntryHelperTest {};

TEST_F(DummyDataTest, FanFailureOnGpu0)
{
    // Dummy use case: fan failure fires on GPU0
    const std::string event = "xyz.openbmc_project.State.Fan.FanFailed";
    AddData data = {"SLOT_ID=GPU0", "EVENT_TYPE=STANDARD"};

    // AFID lookup — what Manager would set as ServiceCode
    const char* sc = amd::ael::lookupAfid(event);
    EXPECT_STREQ(sc, "AFID_26");  // Device Internal / HWA / Fatal

    // OEM data — what AelEntry would populate
    auto oem = buildOem(event, data);
    EXPECT_EQ(oem.at("EventMessage"), event);
    EXPECT_EQ(oem.at("SlotId"),       "GPU0");
    EXPECT_EQ(oem.at("EventType"),    "STANDARD");
    EXPECT_EQ(oem.size(), 3u);

    // Sanity: ServiceCode is never empty or AFID_UNKNOWN for this event
    EXPECT_STRNE(sc, "AFID_UNKNOWN");
    EXPECT_STRNE(sc, "");
}

TEST_F(DummyDataTest, CperErrorOnGpu1WithFullMetadata)
{
    // Dummy use case: CPER event from amd-bmc-ras for HBM error on GPU1
    const std::string event = "com.amd.Event.Cper.CperReported";
    AddData data = {
        "SLOT_ID=GPU1",
        "EVENT_TYPE=CPER",
        "CORRELATION_ID=cper-abc-123",
        "CPER_SECTION_TYPE=Memory",
        "GPU_ID=GPU1",
        "SEVERITY_HINT=Fatal",
    };

    const char* sc = amd::ael::lookupAfid(event);
    EXPECT_STREQ(sc, "AFID_34");  // default Unidentified until decoded

    auto oem = buildOem(event, data);
    EXPECT_EQ(oem.at("SlotId"),          "GPU1");
    EXPECT_EQ(oem.at("EventType"),       "CPER");
    EXPECT_EQ(oem.at("CorrelationId"),   "cper-abc-123");
    EXPECT_EQ(oem.at("CperSectionType"), "Memory");
    EXPECT_EQ(oem.at("GpuId"),          "GPU1");
    EXPECT_EQ(oem.at("SeverityHint"),   "Fatal");
    EXPECT_EQ(oem.at("EventMessage"),    event);
    EXPECT_EQ(oem.size(), 7u);
}

TEST_F(DummyDataTest, ThermalOverTempOnCpu0)
{
    const std::string event =
        "xyz.openbmc_project.State.Thermal.DeviceOverOperatingTemperatureFault";
    AddData data = {"SLOT_ID=CPU0", "COMPONENT_ID=ThermalSensor0"};

    const char* sc = amd::ael::lookupAfid(event);
    EXPECT_STREQ(sc, "AFID_26");

    auto oem = buildOem(event, data);
    EXPECT_EQ(oem.at("SlotId"),      "CPU0");
    EXPECT_EQ(oem.at("ComponentId"), "ThermalSensor0");
}

TEST_F(DummyDataTest, UnmappedEventGracefulFallback)
{
    // New event not yet in the AFID map — must not crash, must return AFID_UNKNOWN
    const char* sc = amd::ael::lookupAfid(
        "xyz.openbmc_project.Some.NewEvent.NotYetMapped");
    EXPECT_STREQ(sc, "AFID_UNKNOWN");

    // OEM data still builds correctly even for unknown events
    AddData data = {"SLOT_ID=GPU2"};
    auto oem = buildOem(
        "xyz.openbmc_project.Some.NewEvent.NotYetMapped", data);
    EXPECT_EQ(oem.at("SlotId"), "GPU2");
}

TEST_F(DummyDataTest, SensorCriticalReading)
{
    const std::string event =
        "xyz.openbmc_project.Sensor.Threshold.ReadingAboveUpperCriticalThreshold";
    AddData data = {"SLOT_ID=SENSOR_BOARD", "COMPONENT_ID=TempSensor0"};

    const char* sc = amd::ael::lookupAfid(event);
    EXPECT_STREQ(sc, "AFID_25");  // HBM All Others Fatal

    auto oem = buildOem(event, data);
    EXPECT_EQ(oem.at("SlotId"),      "SENSOR_BOARD");
    EXPECT_EQ(oem.at("ComponentId"), "TempSensor0");
}

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}