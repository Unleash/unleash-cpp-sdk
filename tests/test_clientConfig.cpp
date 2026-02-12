#include <gtest/gtest.h>

#include <chrono>
#include <map>
#include <string>

#include "unleash/Configuration/clientConfig.hpp"
#include "unleash/Domain/toggleSet.hpp"
#include "unleash/Domain/toggle.hpp"



using namespace unleash;


TEST(ClientConfig, ConstructorStoresMandatoryFields)
{
    std::string url = "http://example";
    std::string key = "key123";
    std::string appName = "cppApp";
    
    ClientConfig cfg(url, key, appName);

    EXPECT_EQ(cfg.url(), "http://example");
    EXPECT_EQ(cfg.clientKey(), "key123");
    EXPECT_EQ(cfg.appName(), "cppApp");
}

TEST(ClientConfig, OnEmptyAppNameDefaultNameUsed)
{
    std::string url = "http://example";
    std::string key = "key123";
    std::string appName = "";
    
    ClientConfig cfg(url, key, appName);
    EXPECT_EQ(cfg.appName(), std::string(utils::defaultAppName));
}

TEST(ClientConfig, DefaultRefreshAndMetricsMatchExpectedBehavior)
{
    std::string url = "http://example";
    std::string key = "key123";
    std::string appName = "cppApp";
    
    ClientConfig cfg(url, key, appName);

    // Defaults from your header:
    EXPECT_EQ(cfg.refreshInterval().count(), 0);
    EXPECT_FALSE(cfg.isRefreshEnabled());

    EXPECT_EQ(cfg.metricsInterval().count(), 0);
    EXPECT_FALSE(cfg.isMetricsEnabled());
}

TEST(ClientConfig, RefreshEnabledRespectsDisableRefreshAndZeroInterval)
{
    std::string url = "http://example";
    std::string key = "key123";
    std::string appName = "cppApp";
    
    ClientConfig cfg(url, key, appName);

    EXPECT_FALSE(cfg.isRefreshEnabled());

    cfg.setRefreshInterval(utils::seconds(5));
    EXPECT_TRUE(cfg.isRefreshEnabled());
}

TEST(ClientConfig, MetricsEnabledRespectsDisableMetricsAndZeroInterval)
{
    std::string url = "http://example";
    std::string key = "key123";
    std::string appName = "cppApp";
    
    ClientConfig cfg(url, key, appName);

    EXPECT_FALSE(cfg.isMetricsEnabled());

    cfg.setMetricsInterval(utils::seconds(5));
    EXPECT_TRUE(cfg.isMetricsEnabled());
}

TEST(ClientConfig, HeaderAndCustomHeadersDefaultsAndSetters)
{
    std::string url = "http://example";
    std::string key = "key123";
    std::string appName = "cppApp";
    
    ClientConfig cfg(url, key, appName);
    EXPECT_EQ(cfg.headerName(), std::string(utils::defaultHeadeName));
    EXPECT_TRUE(cfg.customHeaders().empty());

        std::map<std::string, std::string> h;
    h["X-Test"] = "123";
    h["X-Client"] = "cpp";
    cfg.setHeaderName("X-Auth")
       .setCustomHeaders(h);

    EXPECT_EQ(cfg.headerName(), "X-Auth");
    EXPECT_EQ(cfg.customHeaders().at("X-Test"), "123");
    EXPECT_EQ(cfg.customHeaders().at("X-Client"), "cpp");

}

TEST(ClientConfig, ImpressionAndPostRequestsFlags)
{
    std::string url = "http://example";
    std::string key = "key123";
    std::string appName = "cppApp";
    
    ClientConfig cfg(url, key, appName);

    EXPECT_FALSE(cfg.impressionDataAll());
    EXPECT_FALSE(cfg.usePostRequests());

    cfg.setImpressionDataAll(true)
       .setUsePostRequests(true);

    EXPECT_TRUE(cfg.impressionDataAll());
    EXPECT_TRUE(cfg.usePostRequests());
}

TEST(ClientConfig, BootstrapOptionalAbsentByDefault)
{
    std::string url = "http://example";
    std::string key = "key123";
    std::string appName = "cppApp";
    
    ClientConfig cfg(url, key, appName);

    EXPECT_FALSE(cfg.bootstrap().has_value());
    EXPECT_TRUE(cfg.bootstrapOverride()); // default true in your header
}

TEST(ClientConfig, BootstrapStoresMapAndDoesNotGetEmptiedByToggleSetCtor)
{
    std::string url = "http://example";
    std::string key = "key123";
    std::string appName = "cppApp";
    
    ClientConfig cfg(url, key, appName);

    ToggleSet::Map m;
    m.emplace("flagA", Toggle("flagA", true, false, Variant::disabledFactory()));
    m.emplace("flagB", Toggle("flagB", false, false, Variant::disabledFactory()));

    cfg.setBootstrap(Bootstrap(std::move(m)));

    ASSERT_TRUE(cfg.bootstrap().has_value());
    const auto& ref = cfg.bootstrap()->getToggles();
    ASSERT_EQ(ref.size(), 2u);

    // This should NOT empty Bootstrap map because ToggleSet takes Map by value (copy-then-move).
    ToggleSet snapshot(ref);

    EXPECT_EQ(cfg.bootstrap()->getToggles().size(), 2u);
}

TEST(ClientConfig, ValidateRejectsEmptyMandatoryFields)
{
    std::string url = "";
    std::string key = "";
    std::string appName = "";
    
    ClientConfig cfg(url, key, appName);
    EXPECT_FALSE(cfg.isValid());
}

TEST(ClientConfig, ValidateRejectsNegativeIntervals)
{
    std::string url = "http://example";
    std::string key = "key123";
    std::string appName = "cppApp";
    
    ClientConfig cfg(url, key, appName);
    cfg.setRefreshInterval(utils::seconds{-1});
    EXPECT_FALSE(cfg.isValid());
}

TEST(ClientConfig, ValidateAcceptsDefaultCorrectConfig)
{
    std::string url = "http://example";
    std::string key = "key123";
    std::string appName = "cppApp";
    
    ClientConfig cfg(url, key, appName);
    EXPECT_TRUE(cfg.isValid());
}