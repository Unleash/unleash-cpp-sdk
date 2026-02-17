#include <gtest/gtest.h>

#include "unleash/Utils/jsonCodec.hpp"
#include "unleash/Domain/variant.hpp"
#include "unleash/Domain/context.hpp"

using unleash::JsonCodec;
using unleash::ToggleSet;
using unleash::Variant;
using unleash::Context;
using nlohmann::json;

TEST(JsonCodecDecodeClientFeaturesResponse, ParsesValidTogglesAndVariantsWithPayload)
{
    const std::string jsonText = R"json(
    {
      "toggles": [
        {
          "name": "test-flag",
          "enabled": true,
          "variant": {
            "name": "hello",
            "enabled": true,
            "payload": { "type": "number", "value": "1234" },
            "feature_enabled": true,
            "featureEnabled": true
          },
          "impressionData": false
        },
        {
          "name": "test-json",
          "enabled": true,
          "variant": {
            "name": "hello",
            "enabled": true,
            "payload": { "type": "string", "value": "world" },
            "feature_enabled": true,
            "featureEnabled": true
          },
          "impressionData": false
        }
      ]
    }
    )json";

    ToggleSet set = JsonCodec::decodeClientFeaturesResponse(jsonText);
    std::cout<<"##########################################size is: "<<set.size()<<std::endl;
    EXPECT_EQ(set.size(), 2u);

    EXPECT_TRUE(set.contains("test-flag"));
    EXPECT_TRUE(set.isEnabled("test-flag"));

    Variant v1 = set.getVariant("test-flag");
    EXPECT_EQ(v1.name(), "hello");
    EXPECT_TRUE(v1.enabled());
    EXPECT_TRUE(v1.hasPayload());
    ASSERT_TRUE(v1.payload().has_value());
    EXPECT_EQ(v1.payload()->type(), "number");
    EXPECT_EQ(v1.payload()->value(), "1234");

    EXPECT_TRUE(set.contains("test-json"));
    EXPECT_TRUE(set.isEnabled("test-json"));

    Variant v2 = set.getVariant("test-json");
    EXPECT_EQ(v2.name(), "hello");
    EXPECT_TRUE(v2.enabled());
    EXPECT_TRUE(v2.hasPayload());
    ASSERT_TRUE(v2.payload().has_value());
    EXPECT_EQ(v2.payload()->type(), "string");
    EXPECT_EQ(v2.payload()->value(), "world");
}

TEST(JsonCodecDecodeClientFeaturesResponse, ToggleDisabledForcesDisabledVariant)
{
    const std::string jsonText = R"json(
    {
      "toggles": [
        {
          "name": "flag-off",
          "enabled": false,
          "variant": {
            "name": "hello",
            "enabled": true,
            "payload": { "type": "string", "value": "world" }
          },
          "impressionData": true
        }
      ]
    }
    )json";

    ToggleSet set = JsonCodec::decodeClientFeaturesResponse(jsonText);

    EXPECT_EQ(set.size(), 1u);
    EXPECT_TRUE(set.contains("flag-off"));
    EXPECT_FALSE(set.isEnabled("flag-off"));

    // Your decoder explicitly forces disabled variant if toggle is disabled
    Variant v = set.getVariant("flag-off");
    EXPECT_EQ(v, Variant::disabledFactory());
}

TEST(JsonCodecDecodeClientFeaturesResponse, MissingVariantFallsBackToDisabledVariant)
{
    const std::string jsonText = R"json(
    {
      "toggles": [
        {
          "name": "no-variant",
          "enabled": true,
          "impressionData": false
        }
      ]
    }
    )json";

    ToggleSet set = JsonCodec::decodeClientFeaturesResponse(jsonText);

    EXPECT_EQ(set.size(), 1u);
    EXPECT_TRUE(set.contains("no-variant"));
    EXPECT_TRUE(set.isEnabled("no-variant"));

    Variant v = set.getVariant("no-variant");
    EXPECT_EQ(v, Variant::disabledFactory());
}

TEST(JsonCodecDecodeClientFeaturesResponse, InvalidJsonReturnsEmptyToggleSet)
{
    const std::string invalid = R"({ "toggles": [ )";

    ToggleSet set = JsonCodec::decodeClientFeaturesResponse(invalid);

    EXPECT_EQ(set.size(), 0u);
}

TEST(JsonCodecDecodeClientFeaturesResponse, MissingTogglesFieldReturnsEmptyToggleSet)
{
    const std::string jsonText = R"json({ "foo": 123 })json";

    ToggleSet set = JsonCodec::decodeClientFeaturesResponse(jsonText);

    EXPECT_EQ(set.size(), 0u);
}

TEST(JsonCodecDecodeClientFeaturesResponse, DuplicateToggleNamesFirstOneWins)
{
    // Your ToggleSet(vector&&) documentation says: First one wins.
    const std::string jsonText = R"json(
    {
      "toggles": [
        { "name": "dup", "enabled": true,  "variant": { "name": "A", "enabled": true } },
        { "name": "dup", "enabled": false, "variant": { "name": "B", "enabled": true } }
      ]
    }
    )json";

    ToggleSet set = JsonCodec::decodeClientFeaturesResponse(jsonText);

    EXPECT_TRUE(set.contains("dup"));
    EXPECT_TRUE(set.isEnabled("dup"));             // should reflect the first element
    EXPECT_EQ(set.getVariant("dup").name(), "A");  // variant from first wins too
}

TEST(JsonCodecEncodeContextRequestBody, ProducesValidJsonWithContextRoot)
{
    Context ctx("unleash-demo2", "24876069");

    const std::string body = JsonCodec::encodeContextRequestBody(ctx);

    nlohmann::json j = nlohmann::json::parse(body);
    ASSERT_TRUE(j.is_object());
    ASSERT_TRUE(j.contains("context"));
    ASSERT_TRUE(j["context"].is_object());
}

TEST(JsonCodecEncodeContextRequestBody, WritesAppNameEnvironmentSessionIdToCorrectFields)
{
    //Context ctx("unleash-demo2", "24876069");
    Context ctx("unleash-demo2", std::string(),  "24876069");
    const std::string body = JsonCodec::encodeContextRequestBody(ctx);
    nlohmann::json j = nlohmann::json::parse(body);

    const auto& c = j["context"];
    EXPECT_EQ(c.value("appName", ""), "unleash-demo2");
    EXPECT_EQ(c.value("sessionId", ""), "24876069");
}

TEST(JsonCodecEncodeContextRequestBody, DonTOmitsSessionIdIfEmpty)
{
    Context ctx("app");

    const std::string body = JsonCodec::encodeContextRequestBody(ctx);
    nlohmann::json j = nlohmann::json::parse(body);

    const auto& c = j["context"];
    EXPECT_EQ(c.value("appName", ""), "app");

    if (c.contains("sessionId")) {
        EXPECT_TRUE(c["sessionId"].is_string());
        EXPECT_FALSE(c["sessionId"].get<std::string>().empty());
    }
}

TEST(JsonCodecEncodeContextRequestBody, SerializesOptionalFieldsWhenSet)
{
    Context ctx("app",  "s1");
    ctx.setUserId("u42")
       .setRemoteAddress("10.0.0.1")
       .setCurrentTime();

    const std::string body = JsonCodec::encodeContextRequestBody(ctx);
    nlohmann::json j = nlohmann::json::parse(body);

    const auto& c = j["context"];
    EXPECT_EQ(c.value("userId", ""), "u42");
    EXPECT_EQ(c.value("remoteAddress", ""), "10.0.0.1");
    EXPECT_FALSE(c.value("currentTime", "").empty());
}

TEST(JsonCodecEncodeContextRequestBody, SerializesPropertiesIfPresent)
{
    Context ctx("app", "s1");
    ctx.setProperty("tenant", "acme")
       .setProperty("plan", "pro");

    const std::string body = JsonCodec::encodeContextRequestBody(ctx);
    std::cout<<"Body is: "<<body<<std::endl;
    nlohmann::json j = nlohmann::json::parse(body);
    
    const auto& c = j["context"];

    ASSERT_TRUE(c.contains("properties"));
    ASSERT_TRUE(c["properties"].is_object());

    EXPECT_EQ(c["properties"].value("tenant", ""), "acme");
    EXPECT_EQ(c["properties"].value("plan", ""), "pro");
}

