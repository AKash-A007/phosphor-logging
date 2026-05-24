// SPDX-License-Identifier: Apache-2.0
// test/ael/test_ael.cpp
//
// Google Test suite for AEL (AMD Event Logging) extension.
//
// Tests cover:
//   1. AFID lookup from generated header (event_afid_map.hpp)
//   2. AdditionalData key extraction
//   3. OemAdditionalData map building
//   4. Known keys mapped to correct camelCase names
//   5. Unknown keys ignored
//   6. AFID_UNKNOWN for unmapped events
//   7. Empty additionalData
//   8. Multiple keys in one call
//   9. CPER event flow end to end
//  10. Dummy data simple use case (what mentor asked for)

#include "extensions/ael/ael.hpp"
#include "event_afid_map.hpp"

#include <gtest/gtest.h>

#include <map>
#include <string>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// Helper: simulate AdditionalDataArg (vector of "KEY=VALUE" strings)
// ─────────────────────────────────────────────────────────────────────────────

using AddData = std::vector<std::string>;

// ─────────────────────────────────────────────────────────────────────────────
// 1. AFID Lookup Tests
// ─────────────────────────────────────────────────────────────────────────────

class AfidLookupTest : public ::testing::Test {};

TEST_F(AfidLookupTest, FanFailedReturnsCorrectAfid)
{
    const char* afid = amd::ael::lookupAfid(
        "xyz.openbmc_project.State.Fan.FanFailed");
    EXPECT_STREQ(afid, "AFID_26");
}

TEST_F(AfidLookupTest, FanRestoredReturnsCorrectAfid)
{
    const char* afid = amd::ael::lookupAfid(
        "xyz.openbmc_project.State.Fan.FanRestored");
    EXPECT_STREQ(afid, "AFID_29");
}

TEST_F(AfidLookupTest, CperReportedReturnsCorrectAfid)
{
    const char* afid = amd::ael::lookupAfid(
        "com.amd.Event.Cper.CperReported");
    EXPECT_STREQ(afid, "AFID_34");
}

TEST_F(AfidLookupTest, ThermalFaultReturnsCorrectAfid)
{
    const char* afid = amd::ael::lookupAfid(
        "xyz.openbmc_project.State.Thermal.DeviceOverOperatingTemperatureFault");
    EXPECT_STREQ(afid, "AFID_26");
}

TEST_F(AfidLookupTest, SensorCriticalReturnsCorrectAfid)
{
    const char* afid = amd::ael::lookupAfid(
        "xyz.openbmc_project.Sensor.Threshold.ReadingAboveUpperCriticalThreshold");
    EXPECT_STREQ(afid, "AFID_25");
}

TEST_F(AfidLookupTest, UnknownEventReturnsAfidUnknown)
{
    const char* afid = amd::ael::lookupAfid(
        "xyz.openbmc_project.Some.Event.ThatDoesNotExist");
    EXPECT_STREQ(afid, "AFID_UNKNOWN");
}

TEST_F(AfidLookupTest, EmptyStringReturnsAfidUnknown)
{
    const char* afid = amd::ael::lookupAfid("");
    EXPECT_STREQ(afid, "AFID_UNKNOWN");
}

// ─────────────────────────────────────────────────────────────────────────────
// 2. AdditionalData Key Extraction Tests
// ─────────────────────────────────────────────────────────────────────────────

class ExtractKeyTest : public ::testing::Test {};

TEST_F(ExtractKeyTest, ExtractsSlotId)
{
    AddData data = {"SLOT_ID=GPU0", "EVENT_TYPE=CPER"};
    auto val = amd::ael::extractAdditionalDataValue(data, "SLOT_ID");
    EXPECT_EQ(val, "GPU0");
}

TEST_F(ExtractKeyTest, ExtractsEventType)
{
    AddData data = {"EVENT_TYPE=STANDARD", "SLOT_ID=GPU1"};
    auto val = amd::ael::extractAdditionalDataValue(data, "EVENT_TYPE");
    EXPECT_EQ(val, "STANDARD");
}

TEST_F(ExtractKeyTest, ReturnsEmptyForMissingKey)
{
    AddData data = {"SLOT_ID=GPU0"};
    auto val = amd::ael::extractAdditionalDataValue(data, "CORRELATION_ID");
    EXPECT_TRUE(val.empty());
}

TEST_F(ExtractKeyTest, ReturnsEmptyForEmptyData)
{
    AddData data = {};
    auto val = amd::ael::extractAdditionalDataValue(data, "SLOT_ID");
    EXPECT_TRUE(val.empty());
}

TEST_F(ExtractKeyTest, HandlesValueWithEqualsSign)
{
    // Value itself contains '=' — should only strip first prefix
    AddData data = {"CORRELATION_ID=abc=123=xyz"};
    auto val = amd::ael::extractAdditionalDataValue(data, "CORRELATION_ID");
    EXPECT_EQ(val, "abc=123=xyz");
}

TEST_F(ExtractKeyTest, DoesNotPartiallyMatchKey)
{
    AddData data = {"SLOT_ID_EXTRA=should_not_match"};
    auto val = amd::ael::extractAdditionalDataValue(data, "SLOT_ID");
    EXPECT_TRUE(val.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// 3. OemAdditionalData Building Tests
// ─────────────────────────────────────────────────────────────────────────────

class BuildOemDataTest : public ::testing::Test {};

TEST_F(BuildOemDataTest, AlwaysIncludesEventMessage)
{
    AddData data = {};
    auto oem = amd::ael::buildOemData(
        "xyz.openbmc_project.State.Fan.FanFailed", data);

    EXPECT_EQ(oem.count("EventMessage"), 1);
    EXPECT_EQ(oem.at("EventMessage"),
              "xyz.openbmc_project.State.Fan.FanFailed");
}

TEST_F(BuildOemDataTest, SlotIdMappedToCamelCase)
{
    AddData data = {"SLOT_ID=GPU0"};
    auto oem = amd::ael::buildOemData("some.event", data);
    EXPECT_EQ(oem.at("SlotId"), "GPU0");
    EXPECT_EQ(oem.count("SLOT_ID"), 0); // raw key must NOT appear
}

TEST_F(BuildOemDataTest, EventTypeMappedToCamelCase)
{
    AddData data = {"EVENT_TYPE=CPER"};
    auto oem = amd::ael::buildOemData("some.event", data);
    EXPECT_EQ(oem.at("EventType"), "CPER");
}

TEST_F(BuildOemDataTest, CorrelationIdMappedToCamelCase)
{
    AddData data = {"CORRELATION_ID=abc-123"};
    auto oem = amd::ael::buildOemData("some.event", data);
    EXPECT_EQ(oem.at("CorrelationId"), "abc-123");
}

TEST_F(BuildOemDataTest, CperSectionTypeMappedToCamelCase)
{
    AddData data = {"CPER_SECTION_TYPE=Memory"};
    auto oem = amd::ael::buildOemData("some.event", data);
    EXPECT_EQ(oem.at("CperSectionType"), "Memory");
}

TEST_F(BuildOemDataTest, UnknownKeysIgnored)
{
    AddData data = {"SLOT_ID=GPU0", "RANDOM_UNKNOWN_KEY=value",
                    "ANOTHER_KEY=ignored"};
    auto oem = amd::ael::buildOemData("some.event", data);

    EXPECT_EQ(oem.count("SlotId"), 1);
    EXPECT_EQ(oem.count("RANDOM_UNKNOWN_KEY"), 0);
    EXPECT_EQ(oem.count("ANOTHER_KEY"), 0);
}

TEST_F(BuildOemDataTest, AllKnownKeysTogether)
{
    AddData data = {
        "SLOT_ID=GPU1",
        "EVENT_TYPE=CPER",
        "CORRELATION_ID=corr-456",
        "COMPONENT_ID=HBM0",
        "CPER_SECTION_TYPE=Memory",
        "GPU_ID=GPU1",
        "SEVERITY_HINT=Fatal",
    };
    auto oem = amd::ael::buildOemData("com.amd.Event.Cper.CperReported", data);

    EXPECT_EQ(oem.at("SlotId"),          "GPU1");
    EXPECT_EQ(oem.at("EventType"),       "CPER");
    EXPECT_EQ(oem.at("CorrelationId"),   "corr-456");
    EXPECT_EQ(oem.at("ComponentId"),     "HBM0");
    EXPECT_EQ(oem.at("CperSectionType"), "Memory");
    EXPECT_EQ(oem.at("GpuId"),          "GPU1");
    EXPECT_EQ(oem.at("SeverityHint"),   "Fatal");
    EXPECT_EQ(oem.at("EventMessage"),    "com.amd.Event.Cper.CperReported");
}

TEST_F(BuildOemDataTest, EmptyDataOnlyHasEventMessage)
{
    AddData data = {};
    auto oem = amd::ael::buildOemData("xyz.openbmc_project.State.Fan.FanFailed",
                                       data);
    EXPECT_EQ(oem.size(), 1);
    EXPECT_EQ(oem.count("EventMessage"), 1);
}

// ─────────────────────────────────────────────────────────────────────────────
// 4. Dummy Data Simple Use Case
//    This is what your mentor asked for — a simple end-to-end test
//    that shows the full flow with known dummy data
// ─────────────────────────────────────────────────────────────────────────────

class DummyDataUseCaseTest : public ::testing::Test {};

TEST_F(DummyDataUseCaseTest, FanFailureFullFlow)
{
    // Simulate: a fan failure event fires on GPU0 slot
    const std::string eventId = "xyz.openbmc_project.State.Fan.FanFailed";
    AddData addData = {
        "SLOT_ID=GPU0",
        "EVENT_TYPE=STANDARD",
    };

    // Step 1: AFID lookup
    const char* serviceCode = amd::ael::lookupAfid(eventId);
    EXPECT_STREQ(serviceCode, "AFID_26"); // Device Internal / HWA / Fatal

    // Step 2: Build OEM data
    auto oemData = amd::ael::buildOemData(eventId, addData);

    // Step 3: Verify what would go into Entry.Oem
    EXPECT_EQ(oemData.at("EventMessage"), eventId);
    EXPECT_EQ(oemData.at("SlotId"),       "GPU0");
    EXPECT_EQ(oemData.at("EventType"),    "STANDARD");

    // Step 4: Verify serviceCode + oemData would be correct for D-Bus
    EXPECT_FALSE(std::string(serviceCode).empty());
    EXPECT_NE(std::string(serviceCode), "AFID_UNKNOWN");
    EXPECT_GE(oemData.size(), 3u); // EventMessage + SlotId + EventType
}

TEST_F(DummyDataUseCaseTest, CperEventFullFlow)
{
    // Simulate: CPER event from amd-bmc-ras for HBM memory error on GPU1
    const std::string eventId = "com.amd.Event.Cper.CperReported";
    AddData addData = {
        "SLOT_ID=GPU1",
        "EVENT_TYPE=CPER",
        "CORRELATION_ID=cper-session-789",
        "CPER_SECTION_TYPE=Memory",
        "GPU_ID=GPU1",
        "SEVERITY_HINT=Fatal",
    };

    // AFID lookup — default AFID_34 until CPER is decoded
    const char* serviceCode = amd::ael::lookupAfid(eventId);
    EXPECT_STREQ(serviceCode, "AFID_34"); // Unidentified (default for CPER)

    // Build OEM data
    auto oemData = amd::ael::buildOemData(eventId, addData);

    // Verify all CPER fields present
    EXPECT_EQ(oemData.at("EventMessage"),    eventId);
    EXPECT_EQ(oemData.at("SlotId"),          "GPU1");
    EXPECT_EQ(oemData.at("EventType"),       "CPER");
    EXPECT_EQ(oemData.at("CorrelationId"),   "cper-session-789");
    EXPECT_EQ(oemData.at("CperSectionType"), "Memory");
    EXPECT_EQ(oemData.at("GpuId"),          "GPU1");
    EXPECT_EQ(oemData.at("SeverityHint"),   "Fatal");
}

TEST_F(DummyDataUseCaseTest, ThermalOverTempFullFlow)
{
    const std::string eventId =
        "xyz.openbmc_project.State.Thermal.DeviceOverOperatingTemperatureFault";
    AddData addData = {
        "SLOT_ID=CPU0",
        "EVENT_TYPE=STANDARD",
        "COMPONENT_ID=ThermalSensor0",
    };

    const char* serviceCode = amd::ael::lookupAfid(eventId);
    EXPECT_STREQ(serviceCode, "AFID_26");

    auto oemData = amd::ael::buildOemData(eventId, addData);
    EXPECT_EQ(oemData.at("SlotId"),      "CPU0");
    EXPECT_EQ(oemData.at("ComponentId"), "ThermalSensor0");
}

TEST_F(DummyDataUseCaseTest, UnmappedEventGetsAfidUnknown)
{
    // Any event not in the map must gracefully return AFID_UNKNOWN
    const char* afid = amd::ael::lookupAfid(
        "xyz.openbmc_project.Some.NewEvent.NotYetMapped");
    EXPECT_STREQ(afid, "AFID_UNKNOWN");

    // OEM data should still be built correctly even for unknown events
    AddData addData = {"SLOT_ID=GPU2"};
    auto oemData = amd::ael::buildOemData(
        "xyz.openbmc_project.Some.NewEvent.NotYetMapped", addData);
    EXPECT_EQ(oemData.at("SlotId"), "GPU2");
}

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}