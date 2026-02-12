#pragma once

#include <optional>
#include <string>
#include <utility>
#include <chrono>
#include <map>


#include "unleash/Domain/context.hpp"
#include "unleash/Domain/toggleSet.hpp"
#include "unleash/Utils/utils.hpp"



namespace unleash 
{

class Bootstrap final {
public:
    explicit Bootstrap(ToggleSet::Map p_map);
    const ToggleSet::Map& getToggles() const;
private: 
    ToggleSet::Map _map;
};

class ClientConfig final {

public:

    
    ClientConfig(const std::string& p_url, const std::string& p_clientKey, const std::string& p_appName);
    //setters:
    ClientConfig& setInstanceId(const std::string& p_instanceId);
    ClientConfig& setRefreshInterval(utils::seconds s);
    ClientConfig& setMetricsInterval(utils::seconds s);
    ClientConfig& setMetricsIntervalInitial(utils::seconds s);
    ClientConfig& setBootstrap(Bootstrap b);
    ClientConfig& setBootstrapOverride(bool v);
    ClientConfig& setHeaderName(std::string headerName);
    ClientConfig& setCustomHeaders(std::map<std::string, std::string> headers);
    ClientConfig& setImpressionDataAll(bool v);
    ClientConfig& setUsePostRequests(bool v);
    ClientConfig& setTimeOutQuery(utils::mSeconds m);
    ClientConfig& setTogglesStorageTTL(utils::seconds ttl);


    //getters: 
    const std::string& url() const;
    const std::string& clientKey() const;
    const std::string& appName() const;
    const std::string& connectionId() const;
    const std::string& instanceId() const;
    utils::seconds refreshInterval() const;
    utils::seconds metricsInterval() const;
    utils::seconds metricsIntervalInitial() const;
    const std::optional<Bootstrap>& bootstrap() const;
    bool bootstrapOverride() const;
    const std::string& headerName() const;
    const std::map<std::string, std::string>& customHeaders() const;
    bool impressionDataAll() const;
    bool usePostRequests() const;
    utils::mSeconds timeOutQuery() const;
    utils::seconds togglesStorageTTL() const;

    bool isRefreshEnabled() const;
    bool isMetricsEnabled() const;
    

    bool isValid();
private: 

    std::string _url;
    std::string _clientKey;
    std::string _appName;
    std::string _connectionId;

    utils::seconds _refreshInterval{0};
    utils::seconds _metricsInterval{0};
    utils::seconds _metricsIntervalInitial{0};
    std::optional<Bootstrap> _bootstrap{};
    bool _bootstrapOverride{true};
    std::string _headerName =  std::string(utils::defaultHeadeName);
    std::map<std::string, std::string> _customHeaders{};
    bool _impressionDataAll{false};
    bool _usePostRequests{false};
    std::string _instanceId = std::string(utils::defaultInstanceId);
    utils::mSeconds _timeOutQueryMS{5};

    utils::seconds _togglesStorageTTL{0};





};


}// namespace unleash