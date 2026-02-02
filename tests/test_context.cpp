#include <gtest/gtest.h>
#include "unleash/Domain/context.hpp"

using unleash::Context;

TEST(ContextTest, ConstructorSetsRequiredFields)
{
    Context ctx("myApp", "prod", "123568941");
    EXPECT_EQ(ctx.getAppName(), "myApp");
    EXPECT_EQ(ctx.getEnvironment(), "prod");
    EXPECT_EQ(ctx.getSessionId(), "123568941");
}

TEST(ContextTest, OptionalFieldsAreEmptyByDefault)
{
    Context ctx("myApp", "prod");
    auto sessionId  = ctx.getSessionId();

    EXPECT_FALSE(ctx.hasUserId());
    EXPECT_FALSE(ctx.hasRemoteAddress());
    EXPECT_FALSE(ctx.hasCurrentTime());

    EXPECT_FALSE(ctx.getUserId().has_value());
    EXPECT_FALSE(ctx.getRemoteAddress().has_value());
    EXPECT_FALSE(ctx.getCurrentTime().has_value());
    //verify that _sessionId is not empty:
    EXPECT_FALSE(sessionId.empty());
    
}

TEST(ContextTest, SettersSetAndEmptyResets)
{
    Context ctx("myApp", "prod");

    ctx.setUserId("u1");
    ASSERT_TRUE(ctx.hasUserId());
    EXPECT_EQ(*ctx.getUserId(), "u1");

    ctx.setUserId(""); // should reset
    EXPECT_FALSE(ctx.hasUserId());
    EXPECT_FALSE(ctx.getUserId().has_value());
    ctx.setRemoteAddress("10.0.0.1");
    ASSERT_TRUE(ctx.hasRemoteAddress());
    EXPECT_EQ(*ctx.getRemoteAddress(), "10.0.0.1");

    ctx.setRemoteAddress("");
    EXPECT_FALSE(ctx.hasRemoteAddress());
}

TEST(ContextTest, CurrentTimeValidFormatSetsValue)
{
    Context ctx("myApp", "prod");

    // Exactly 24 chars, strict regex in your verifyCurrentTimeFormat:
    // YYYY-MM-DDTHH:MM:SS.mmmZ
    ctx.setCurrentTime("2026-01-26T10:15:30.123Z");
    ASSERT_TRUE(ctx.hasCurrentTime());
    EXPECT_EQ(*ctx.getCurrentTime(), "2026-01-26T10:15:30.123Z");
}

TEST(ContextTest, CurrentTimeInvalidFormatIsIgnored)
{
    Context ctx("myApp", "prod");

    // Missing milliseconds
    ctx.setCurrentTime("2026-01-26T10:15:30Z");
    EXPECT_FALSE(ctx.hasCurrentTime());

    // Wrong length
    ctx.setCurrentTime("2026-01-26T10:15:30.12Z");
    EXPECT_FALSE(ctx.hasCurrentTime());

    // Bad millis range or malformed
    ctx.setCurrentTime("2026-01-26T10:15:30.9999Z");
    EXPECT_FALSE(ctx.hasCurrentTime());
}

TEST(ContextTest, SetPropertyAddsAndUpdates)
{
    Context ctx("myApp", "prod");

    ctx.setProperty("tenant", "acme");
    ASSERT_EQ(ctx.getProperties().size(), 1u);
    EXPECT_EQ(ctx.getProperties()[0].first, "tenant");
    EXPECT_EQ(ctx.getProperties()[0].second, "acme");

    // Update existing key
    ctx.setProperty("tenant", "acme2");
    ASSERT_EQ(ctx.getProperties().size(), 1u);
    EXPECT_EQ(ctx.getProperties()[0].second, "acme2");
}

TEST(ContextTest, SetPropertyIgnoresEmptyAndReservedKeys)
{
    Context ctx("myApp", "prod");

    ctx.setProperty("", "x");
    EXPECT_TRUE(ctx.getProperties().empty());

    // reserved keys should be ignored by your current implementation
    ctx.setProperty("userId", "x");
    ctx.setProperty("appName", "x");
    ctx.setProperty("currentTime", "x");
    EXPECT_TRUE(ctx.getProperties().empty());
}