#include <gtest/gtest.h>

#include "unleash/Metrics/metricToggle.hpp"

#include <string>

using namespace unleash;

TEST(MetricToggleTest, Constructor_InitializesCountsAndVariantStatsCorrectly)
{
    MetricToggle mt("flagA", /*isYes=*/true, "variant1");

    EXPECT_EQ(mt.getToggleName(), "flagA");
    EXPECT_EQ(mt.getYesCount(), 1u);
    EXPECT_EQ(mt.getNoCount(), 0u);

    const auto& stats = mt.getVariantStats();
    ASSERT_EQ(stats.size(), 1u);
    ASSERT_TRUE(stats.find("variant1") != stats.end());
    EXPECT_EQ(stats.at("variant1"), 1u);  // must be 1, not 2
}

TEST(MetricToggleTest, UpdateMetric_IncrementsSameVariantAndYesNoCounters)
{
    MetricToggle mt("flagA", /*isYes=*/true, "variant1");

    mt.updateVariantMetric(/*isYes=*/true, "variant1");
    mt.updateVariantMetric(/*isYes=*/false, "variant1");
    mt.updateVariantMetric(/*isYes=*/false, "variant1");

    EXPECT_EQ(mt.getYesCount(), 2u);
    EXPECT_EQ(mt.getNoCount(), 2u);

    const auto& stats = mt.getVariantStats();
    ASSERT_EQ(stats.size(), 1u);
    EXPECT_EQ(stats.at("variant1"), 4u); // 1 from ctor + 3 updates
}

TEST(MetricToggleTest, UpdateMetric_TracksMultipleVariantsSeparately)
{
    MetricToggle mt("flagA", /*isYes=*/false, "blue");

    mt.updateVariantMetric(/*isYes=*/true,"blue");     // blue++
    mt.updateVariantMetric(/*isYes=*/true, "green");    // green++
    mt.updateVariantMetric(/*isYes=*/false, "green");   // green++
    mt.updateVariantMetric(/*isYes=*/false, "red");     // red++

    EXPECT_EQ(mt.getYesCount(), 2u); // green(true) + blue(true)
    EXPECT_EQ(mt.getNoCount(), 3u);  // ctor(false) + green(false) + red(false)
    mt.updateEnableMetric(false);
    EXPECT_EQ(mt.getNoCount(), 4u);  // the No count is incremented by one

    const auto& stats = mt.getVariantStats();
    ASSERT_EQ(stats.size(), 3u);

    EXPECT_EQ(stats.at("blue"), 2u);   // ctor blue + one update
    EXPECT_EQ(stats.at("green"), 2u);  // two updates
    EXPECT_EQ(stats.at("red"), 1u);    // one update
}
