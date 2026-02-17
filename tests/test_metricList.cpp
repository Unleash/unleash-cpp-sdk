#include <gtest/gtest.h>

#include "unleash/Metrics/metricList.hpp"

#include <string>

using namespace unleash;

TEST(MetricListTest, DefaultConstructedIsEmpty)
{
    MetricList ml;
    EXPECT_TRUE(ml.getList().empty());
}

TEST(MetricListTest, AddMetricData_FirstInsertCreatesToggleAndCountsFirstEvent)
{
    MetricList ml;

    ml.addVariantMetricData("flagA", true, "variant1");

    const auto& m = ml.getList();
    ASSERT_EQ(m.size(), 1u);

    auto it = m.find("flagA");
    ASSERT_NE(it, m.end());

    // MetricToggle inside must carry the proper toggle name (this catches move/order bugs)
    EXPECT_EQ(it->second.getToggleName(), "flagA");

    // First event should be counted once (constructor path)
    EXPECT_EQ(it->second.getYesCount(), 1u);
    EXPECT_EQ(it->second.getNoCount(), 0u);

    const auto& stats = it->second.getVariantStats();
    ASSERT_EQ(stats.size(), 1u);
    EXPECT_EQ(stats.at("variant1"), 1u);
}

TEST(MetricListTest, AddMetricData_SecondCallUpdatesExistingToggle)
{
    MetricList ml;

    // First event: creates entry
    ml.addVariantMetricData("flagA", true, "variant1");

    // Next events: should update existing MetricToggle (not create a second entry)
    ml.addVariantMetricData("flagA", true, "variant1");
    ml.addVariantMetricData("flagA", false, "variant1");
    ml.addVariantMetricData("flagA", false, "variant2");

    const auto& m = ml.getList();
    ASSERT_EQ(m.size(), 1u);

    auto it = m.find("flagA");
    ASSERT_NE(it, m.end());

    EXPECT_EQ(it->second.getToggleName(), "flagA");
    EXPECT_EQ(it->second.getYesCount(), 2u); // ctor yes + update yes
    EXPECT_EQ(it->second.getNoCount(), 2u);  // two update no

    const auto& stats = it->second.getVariantStats();
    ASSERT_EQ(stats.size(), 2u);
    EXPECT_EQ(stats.at("variant1"), 3u); // ctor + 2 updates on variant1
    EXPECT_EQ(stats.at("variant2"), 1u); // 1 update on variant2
}

TEST(MetricListTest, AddMetricData_MultipleTogglesAreTrackedIndependently)
{
    MetricList ml;

    ml.addVariantMetricData("flagA", true, "v1" );
    ml.addVariantMetricData("flagB", false, "blue");
    ml.addVariantMetricData("flagB", true, "blue");
    ml.addVariantMetricData("flagA", false, "v2");

    const auto& m = ml.getList();
    ASSERT_EQ(m.size(), 2u);

    // flagA
    {
        auto it = m.find("flagA");
        ASSERT_NE(it, m.end());
        EXPECT_EQ(it->second.getToggleName(), "flagA");
        EXPECT_EQ(it->second.getYesCount(), 1u);
        EXPECT_EQ(it->second.getNoCount(), 1u);

        const auto& stats = it->second.getVariantStats();
        ASSERT_EQ(stats.size(), 2u);
        EXPECT_EQ(stats.at("v1"), 1u);
        EXPECT_EQ(stats.at("v2"), 1u);
    }

    // flagB
    {
        auto it = m.find("flagB");
        ASSERT_NE(it, m.end());
        EXPECT_EQ(it->second.getToggleName(), "flagB");
        EXPECT_EQ(it->second.getYesCount(), 1u);
        EXPECT_EQ(it->second.getNoCount(), 1u);

        const auto& stats = it->second.getVariantStats();
        ASSERT_EQ(stats.size(), 1u);
        EXPECT_EQ(stats.at("blue"), 2u); // ctor + one update
    }
}
