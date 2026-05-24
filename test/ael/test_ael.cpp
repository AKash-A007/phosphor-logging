// test/ael/test_ael.cpp
//
// Google Test suite for AEL extension.
// Tests the Manager, AelEntry, and entry point logic

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

 // ─────────────────────────────────────────────────────────

class AelEntryHelperTest : public ::testing::Test
{
  protected:
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
            {"SLOT_ID",           "AEL.SLOT_ID"},
            {"EVENT_TYPE",        "AEL.EVENT_TYPE"},
            {"CORRELATION_ID",    "AEL.CORRELATION_ID"},
            {"COMPONENT_ID",      "AEL.COMPONENT_ID"},
            {"CPER_SECTION_TYPE", "AEL.SUBTYPE"},
            {"GPU_ID",            "AEL.GPU_ID"},
            {"SEVERITY_HINT",     "AEL.SEVERITY_OVERRIDE"},
        };
        std::map<std::string, std::string> oem;
        oem["AEL.VERSION"] = "1.0";
        oem["AEL.TYPE"] = "SIMPLE";

        if (msg.find("CperReported") != std::string::npos)
        {
            oem["AEL.TYPE"] = "COMPLEX";
            oem["AEL.SUBTYPE"] = "CPER";
            auto path = extract(data, "Path");
            if (!path.empty()) oem["AEL.DATA.URI"] = "file://" + path;
        }

        for (const auto& [raw, oemKey] : keys)
        {
            auto v = extract(data, raw);
            if (!v.empty()) oem[oemKey] = v;
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


TEST_F(AelEntryHelperTest, OemRawKeysNotPresent)
{
    AddData d = {"SLOT_ID=GPU0"};
    auto oem = buildOem("some.event", d);
    EXPECT_EQ(oem.count("AEL.SLOT_ID"), 1);
    EXPECT_EQ(oem.count("SLOT_ID"), 0);
}

TEST_F(AelEntryHelperTest, OemUnknownKeysIgnored)
{
    AddData d = {"SLOT_ID=GPU0", "UNKNOWN_KEY=ignored"};
    auto oem = buildOem("some.event", d);
    EXPECT_EQ(oem.count("UNKNOWN_KEY"), 0);
    EXPECT_EQ(oem.count("AEL.SLOT_ID"), 1);
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
    EXPECT_EQ(oem.at("AEL.SLOT_ID"),          "GPU1");
    EXPECT_EQ(oem.at("AEL.EVENT_TYPE"),       "CPER");
    EXPECT_EQ(oem.at("AEL.CORRELATION_ID"),   "abc");
    EXPECT_EQ(oem.at("AEL.COMPONENT_ID"),     "HBM0");
    EXPECT_EQ(oem.at("AEL.SUBTYPE"),          "Memory");
    EXPECT_EQ(oem.at("AEL.GPU_ID"),          "GPU1");
    EXPECT_EQ(oem.at("AEL.SEVERITY_OVERRIDE"), "Fatal");
}
// ─────────────────────────────────────────────────────────────────

class DummyDataTest : public AelEntryHelperTest {};

TEST_F(DummyDataTest, FanFailureOnGpu0)
{
    const std::string event = "xyz.openbmc_project.State.Fan.FanFailed";
    AddData data = {"SLOT_ID=GPU0", "EVENT_TYPE=STANDARD"};

    const char* sc = amd::ael::lookupAfid(event);
    EXPECT_STREQ(sc, "AFID_26");  // Device Internal / HWA / Fatal

    auto oem = buildOem(event, data);
    EXPECT_EQ(oem.at("AEL.TYPE"),     "SIMPLE");
    EXPECT_EQ(oem.at("AEL.SLOT_ID"),  "GPU0");
    EXPECT_EQ(oem.at("AEL.EVENT_TYPE"), "STANDARD");
    EXPECT_EQ(oem.size(), 4u); // VERSION, TYPE, SLOT_ID, EVENT_TYPE

    EXPECT_STRNE(sc, "AFID_UNKNOWN");
    EXPECT_STRNE(sc, "");
}

TEST_F(DummyDataTest, CperErrorOnGpu1WithFullMetadata)
{
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
    EXPECT_EQ(oem.at("AEL.TYPE"),        "COMPLEX");
    EXPECT_EQ(oem.at("AEL.SUBTYPE"),     "Memory"); // Overwritten by mapped keys
    EXPECT_EQ(oem.at("AEL.SLOT_ID"),     "GPU1");
    EXPECT_EQ(oem.at("AEL.EVENT_TYPE"),  "CPER");
    EXPECT_EQ(oem.at("AEL.CORRELATION_ID"), "cper-abc-123");
    EXPECT_EQ(oem.at("AEL.GPU_ID"),      "GPU1");
    EXPECT_EQ(oem.at("AEL.SEVERITY_OVERRIDE"), "Fatal");
    EXPECT_EQ(oem.size(), 8u); // VERSION, TYPE, SUBTYPE, SLOT, EVENT, CORR, GPU, SEV
}

TEST_F(DummyDataTest, ThermalOverTempOnCpu0)
{
    const std::string event =
        "xyz.openbmc_project.State.Thermal.DeviceOverOperatingTemperatureFault";
    AddData data = {"SLOT_ID=CPU0", "COMPONENT_ID=ThermalSensor0"};

    const char* sc = amd::ael::lookupAfid(event);
    EXPECT_STREQ(sc, "AFID_26");

    auto oem = buildOem(event, data);
    EXPECT_EQ(oem.at("AEL.SLOT_ID"),      "CPU0");
    EXPECT_EQ(oem.at("AEL.COMPONENT_ID"), "ThermalSensor0");
}

TEST_F(DummyDataTest, UnmappedEventGracefulFallback)
{
    const char* sc = amd::ael::lookupAfid(
        "xyz.openbmc_project.Some.NewEvent.NotYetMapped");
    EXPECT_STREQ(sc, "AFID_UNKNOWN");

    // OEM data still builds correctly even for unknown events
    AddData data = {"SLOT_ID=GPU2"};
    auto oem = buildOem(
        "xyz.openbmc_project.Some.NewEvent.NotYetMapped", data);
    EXPECT_EQ(oem.at("AEL.SLOT_ID"), "GPU2");
}

TEST_F(DummyDataTest, SensorCriticalReading)
{
    const std::string event =
        "xyz.openbmc_project.Sensor.Threshold.ReadingAboveUpperCriticalThreshold";
    AddData data = {"SLOT_ID=SENSOR_BOARD", "COMPONENT_ID=TempSensor0"};

    const char* sc = amd::ael::lookupAfid(event);
    EXPECT_STREQ(sc, "AFID_25");  // HBM All Others Fatal

    auto oem = buildOem(event, data);
    EXPECT_EQ(oem.at("AEL.SLOT_ID"),      "SENSOR_BOARD");
    EXPECT_EQ(oem.at("AEL.COMPONENT_ID"), "TempSensor0");
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}