#pragma once
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <chrono>
#include <vector>
#include <string_view>

namespace unleash {


inline bool isReservedContextKey(std::string_view key) {
    return key == "appName"
        || key == "environment"
        || key == "userId"
        || key == "sessionId"
        || key == "remoteAddress"
        || key == "currentTime";
}

class Context final 
{
public:
    using Properties = std::vector<std::pair<std::string,std::string>>;
    // properties appName, environnement, sessionId are mendatory to set, if p_sessionid is empty resolveSessionId() 
    // will generate a random sessionId following the same algorithm used in th JS equivalent. 
    Context(const std::string& p_appName, const std::string& p_environment, const std::string& p_sessionId = std::string());

    const std::string& getAppName() const;
    const std::string& getEnvironment() const;
    const std::string& getSessionId() const;

    const std::optional<std::string>& getUserId() const;
    const std::optional<std::string>& getRemoteAddress() const;
    const std::optional<std::string>& getCurrentTime() const;

    const Properties& getProperties() const;

    Context& setUserId(const std::string& p_userId);
    Context& setRemoteAddress(const std::string& p_remoteAddress);
    Context& setCurrentTime(const std::string& p_currentTime);

    Context& setProperty(std::string key, std::string value);


    bool hasUserId() const;
    bool hasRemoteAddress() const;
    bool hasCurrentTime() const;

private: 

    bool verifyCurrentTimeFormat(const std::string& timeValue);
    void resolveSessionId();
    std::string _appName; 
    std::string _environment;
    std::string _sessionId;
    std::optional<std::string> _userId;
    std::optional<std::string> _remoteAddress;
    std::optional<std::string>  _currentTime;
    Properties _properties;
};

}// namespace unleash