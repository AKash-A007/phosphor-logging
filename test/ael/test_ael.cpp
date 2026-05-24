// SPDX-License-Identifier: Apache-2.0
// test/ael/test_ael.cpp
//
// Google Test suite for AEL (AMD Event Logging) extension.

#include "extensions/ael/ael.hpp"
#include "event_afid_map.hpp"

#include <gtest/gtest.h>

#include <map>
#include <string>
#include <vector>

using AddData = std::map<std::string, std::string>;

// ─────────────────────────────────────────────────────────────────────────────
// 1. AFID Lookup Tests
// ─────────────────────────────────────────────────────────────────────────────

class AfidLookupTest : public ::testing::Test {};

TEST_F(AfidLookupTest, FanFailedReturnsCorrectAfid)
{
    const char* afid = amd::ael::lookupAfid("xyz.openbmc_project.State.Fan.FanFailed");
    EXPECT_STREQ(afid, "AFID_26");
}

TEST_F(AfidLookupTest, FanRestoredReturnsCorrectAfid)
{
    const char* afid = amd::ael::lookupAfid("xyz.openbmc_project.State.Fan.FanRestored");
    EXPECT_STREQ(afid, "AFID_29");
}

TEST_F(AfidLookupTest, CperReportedReturnsCorrectAfid)
{
    const char* afid = amd::ael::lookupAfid("com.amd.Event.Cper.CperReported");
    EXPECT_STREQ(afid, "AFID_34");
}

TEST_F(AfidLookupTest, ThermalFaultReturnsCorrectAfid)
{
    const char* afid = amd::ael::lookupAfid("xyz.openbmc_project.State.Thermal.DeviceOverOperatingTemperatureFault");
    EXPECT_STREQ(afid, "AFID_26");
}

TEST_F(AfidLookupTest, SensorCriticalReturnsCorrectAfid)
{
    const char* afid = amd::ael::lookupAfid("xyz.openbmc_project.Sensor.Threshold.ReadingAboveUpperCriticalThreshold");
    EXPECT_STREQ(afid, "AFID_25");
}

TEST_F(AfidLookupTest, UnknownEventReturnsAfidUnknown)
{
    const char* afid = amd::ael::lookupAfid("xyz.openbmc_project.Some.Event.ThatDoesNotExist");
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
    AddData data = {{"SLOT_ID", "GPU0"}, {"EVENT_TYPE", "CPER"}};
    auto val = amd::ael::extractAdditionalDataValue(data, "SLOT_ID");
    EXPECT_EQ(val, "GPU0");
}

TEST_F(ExtractKeyTest, ExtractsEventType)
{
    AddData data = {{"EVENT_TYPE", "STANDARD"}, {"SLOT_ID", "GPU1"}};
    auto val = amd::ael::extractAdditionalDataValue(data, "EVENT_TYPE");
    EXPECT_EQ(val, "STANDARD");
}

TEST_F(ExtractKeyTest, ReturnsEmptyForMissingKey)
{
    AddData data = {{"SLOT_ID", "GPU0"}};
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
    AddData data = {{"CORRELATION_ID", "abc=123=xyz"}};
    auto val = amd::ael::extractAdditionalDataValue(data, "CORRELATION_ID");
    EXPECT_EQ(val, "abc=123=xyz");
}

TEST_F(ExtractKeyTest, DoesNotPartiallyMatchKey)
{
    AddData data = {{"SLOT_ID_EXTRA", "should_not_match"}};
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
    auto oem = amd::ael::buildOemData("xyz.openbmc_project.State.Fan.FanFailed", data);
    EXPECT_EQ(oem.at("EventMessage"), "xyz.openbmc_project.State.Fan.FanFailed");
}

TEST_F(BuildOemDataTest, SlotIdMappedToCamelCase)
{
    AddData data = {{"SLOT_ID", "GPU0"}};
    auto oem = amd::ael::buildOemData("some.event", data);
    EXPECT_EQ(oem.at("SlotId"), "GPU0");
    EXPECT_EQ(oem.count("SLOT_ID"), 0);
}

TEST_F(BuildOemDataTest, EventTypeMappedToCamelCase)
{
    AddData data = {{"EVENT_TYPE", "CPER"}};
    auto oem = amd::ael::buildOemData("some.event", data);
    EXPECT_EQ(oem.at("EventType"), "CPER");
}

TEST_F(BuildOemDataTest, CorrelationIdMappedToCamelCase)
{
    AddData data = {{"CORRELATION_ID", "abc-123"}};
    auto oem = amd::ael::buildOemData("some.event", data);
    EXPECT_EQ(oem.at("CorrelationId"), "abc-123");
}

TEST_F(BuildOemDataTest, CperSectionTypeMappedToCamelCase)
{
    AddData data = {{"CPER_SECTION_TYPE", "Memory"}};
    auto oem = amd::ael::buildOemData("some.event", data);
    EXPECT_EQ(oem.at("CperSectionType"), "Memory");
}

TEST_F(BuildOemDataTest, UnknownKeysIgnored)
{
    AddData data = {{"SLOT_ID", "GPU0"}, {"RANDOM_KEY", "val"}};
    auto oem = amd::ael::buildOemData("some.event", data);
    EXPECT_EQ(oem.count("SlotId"), 1);
    EXPECT_EQ(oem.count("RANDOM_KEY"), 0);
}

TEST_F(BuildOemDataTest, AllKnownKeysTogether)
{
    AddData data = {
        {"SLOT_ID", "GPU1"},
        {"EVENT_TYPE", "CPER"},
        {"CORRELATION_ID", "corr-456"},
        {"COMPONENT_ID", "HBM0"},
        {"CPER_SECTION_TYPE", "Memory"},
        {"GPU_ID", "GPU1"},
        {"SEVERITY_HINT", "Fatal"},
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

// ─────────────────────────────────────────────────────────────────────────────
// 4. Dummy Data Simple Use Case
// ─────────────────────────────────────────────────────────────────────────────

class DummyDataUseCaseTest : public ::testing::Test {};

TEST_F(DummyDataUseCaseTest, FanFailureFullFlow)
{
    const std::string eventId = "xyz.openbmc_project.State.Fan.FanFailed";
    AddData addData = {{"SLOT_ID", "GPU0"}, {"EVENT_TYPE", "STANDARD"}};

    const char* serviceCode = amd::ael::lookupAfid(eventId);
    EXPECT_STREQ(serviceCode, "AFID_26");

    auto oemData = amd::ael::buildOemData(eventId, addData);
    EXPECT_EQ(oemData.at("EventMessage"), eventId);
    EXPECT_EQ(oemData.at("SlotId"),       "GPU0");
    EXPECT_EQ(oemData.at("EventType"),    "STANDARD");
}

TEST_F(DummyDataUseCaseTest, CperEventFullFlow)
{
    const std::string eventId = "com.amd.Event.Cper.CperReported";
    AddData data = {
        {"SLOT_ID", "GPU1"},
        {"EVENT_TYPE", "CPER"},
        {"CORRELATION_ID", "cper-123"},
        {"CPER_SECTION_TYPE", "Memory"}
    };

    const char* serviceCode = amd::ael::lookupAfid(eventId);
    EXPECT_STREQ(serviceCode, "AFID_34");

    auto oemData = amd::ael::buildOemData(eventId, data);
    EXPECT_EQ(oemData.at("SlotId"), "GPU1");
    EXPECT_EQ(oemData.at("CorrelationId"), "cper-123");
}

TEST_F(DummyDataUseCaseTest, UnmappedEventGetsAfidUnknown)
{
    const char* afid = amd::ael::lookupAfid("xyz.openbmc_project.Unknown");
    EXPECT_STREQ(afid, "AFID_UNKNOWN");

    AddData addData = {{"SLOT_ID", "GPU2"}};
    auto oemData = amd::ael::buildOemData("xyz.openbmc_project.Unknown", addData);
    EXPECT_EQ(oemData.at("SlotId"), "GPU2");
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}