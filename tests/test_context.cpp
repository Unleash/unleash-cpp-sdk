#include <gtest/gtest.h>
#include "unleash/Domain/context.hpp"
#include "unleash/Utils/utils.hpp"

using unleash::Context;

TEST(ContextTest, ConstructorSetsRequiredFields)
{
    Context ctx("myApp", "123568941");
    EXPECT_EQ(ctx.getAppName(), "myApp");
    EXPECT_EQ(ctx.getSessionId(), "123568941");
}

TEST(ContextTest, ConstructorSetNoField)
{
    Context ctx{};
    EXPECT_EQ(ctx.getAppName(), std::string(utils::defaultAppName));
    EXPECT_FALSE(ctx.getSessionId().empty());
    
}



TEST(ContextTest, OptionalFieldsAreEmptyByDefault)
{
    Context ctx("myApp");
    auto sessionId  = ctx.getSessionId();

    EXPECT_FALSE(ctx.hasUserId());
    EXPECT_FALSE(ctx.hasEnvironment());
    EXPECT_FALSE(ctx.hasRemoteAddress());
    EXPECT_FALSE(ctx.hasCurrentTime());

    EXPECT_FALSE(ctx.getUserId().has_value());
    EXPECT_FALSE(ctx.getEnvironment().has_value());
    EXPECT_FALSE(ctx.getRemoteAddress().has_value());
    EXPECT_FALSE(ctx.getCurrentTime().has_value());
    //verify that _sessionId is not empty:
    EXPECT_FALSE(sessionId.empty());
    
}

TEST(ContextTest, SettersSetAndEmptyResets)
{
    Context ctx("myApp");

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

TEST(ContextTest, SetPropertyAddsAndUpdates)
{
    Context ctx("myApp");

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
    Context ctx("myApp");

    ctx.setProperty("", "x");
    EXPECT_TRUE(ctx.getProperties().empty());

    // reserved keys should be ignored by your current implementation
    ctx.setProperty("userId", "x");
    ctx.setProperty("appName", "x");
    ctx.setProperty("currentTime", "x");
    EXPECT_TRUE(ctx.getProperties().empty());
}

TEST(ContextTest, SetCurrentTimeProperty)
{
    Context ctx("myApp");
    EXPECT_FALSE(ctx.hasCurrentTime());

    ctx.setCurrentTime();
    EXPECT_TRUE(ctx.hasCurrentTime());
}