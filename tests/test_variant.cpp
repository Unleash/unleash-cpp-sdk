#include <gtest/gtest.h>
#include "unleash/Domain/variant.hpp"

using unleash::Variant;

TEST(VariantTest, DefaultConstructedIsDisabledEmptyName)
{
    Variant v;
    EXPECT_FALSE(v.enabled());
    EXPECT_TRUE(v.name().empty());
    EXPECT_FALSE(v.hasPayload());
}

TEST(VariantTest, DisabledFactoryIsConsistent)
{
    Variant v = Variant::disabledFactory();
    EXPECT_EQ(v.name(), "disabled");
    EXPECT_FALSE(v.enabled());
    EXPECT_FALSE(v.hasPayload());
}

TEST(VariantTest, PayloadWorks)
{
    Variant v("blue", true);
    EXPECT_TRUE(v.enabled());
    EXPECT_EQ(v.name(), "blue");
    EXPECT_FALSE(v.hasPayload());

    v.setPayload(Variant::Payload{"string", "hello"});
    ASSERT_TRUE(v.hasPayload());
    EXPECT_EQ(v.payload()->type(), "string");
    EXPECT_EQ(v.payload()->value(), "hello");

    v.clearPayload();
    EXPECT_FALSE(v.hasPayload());
}

TEST(VariantTest, Equality)
{
    Variant a("A", true, Variant::Payload{"string", "x"});
    Variant b("A", true, Variant::Payload{"string", "x"});
    Variant c("A", true, Variant::Payload{"string", "y"});
    Variant d("A", false);

    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
    EXPECT_NE(a, d);
}