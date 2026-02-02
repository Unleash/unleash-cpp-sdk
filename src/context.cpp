#include "unleash/Domain/context.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <regex>
#include <ctime>
#include <random>



namespace unleash 
{

    
Context::Context(const std::string& p_appName, const std::string& p_environment, const std::string& p_sessionId)
                : _appName(p_appName), _environment(p_environment), _sessionId(p_sessionId)
{
    if(_sessionId.empty() ) this->resolveSessionId(); 
}    


bool Context::hasUserId() const {
    return _userId.has_value();
}

bool Context::hasRemoteAddress() const {
    return _remoteAddress.has_value();
}

bool Context::hasCurrentTime() const {
    return _currentTime.has_value();
}


const std::string& Context::getAppName() const       
{ 
    return _appName; 
}
const std::string& Context::getEnvironment() const   
{ 
    return _environment; 
}

const std::string& Context::getSessionId() const
{
    return _sessionId;
}

const std::optional<std::string>& Context::getUserId() const
{
    return _userId;
}

const std::optional<std::string>& Context::getRemoteAddress() const
{
    return _remoteAddress;
}

const std::optional<std::string>& Context::getCurrentTime() const
{
    return _currentTime;
}

const Context::Properties& Context::getProperties() const 
{
    return _properties;
}

Context& Context::setUserId(const std::string& p_userId)
{
    if (p_userId.empty()) _userId.reset();
    else _userId = p_userId;
    return *this;
}

Context& Context::setRemoteAddress(const std::string& p_remoteAddress)
{
    if (p_remoteAddress.empty()) _remoteAddress.reset();
    else _remoteAddress = p_remoteAddress;
    return *this;
}

bool Context::verifyCurrentTimeFormat(const std::string& timeValue)
{
    if (timeValue.length() != 24) {
        return false;
    }

    std::regex pattern(R"(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}\.\d{3}Z)");
    if (!std::regex_match(timeValue, pattern)) {
        return false;
    }

    std::istringstream ss(timeValue);
    std::tm t = {};
    ss >> std::get_time(&t, "%Y-%m-%dT%H:%M:%S");
    
    if (ss.fail()) {
        return false;
    }

    char dot, z;
    int millis;
    ss >> dot >> millis >> z;
    
    return !ss.fail() && dot == '.' && z == 'Z' && 
           millis >= 0 && millis <= 999;
}



Context& Context::setCurrentTime(const std::string& p_currentTime)
{
    if (p_currentTime.empty()) _currentTime.reset();
    if(!verifyCurrentTimeFormat(p_currentTime)) {
        //define a logging strategy here!
        return *this;
    }
    _currentTime = p_currentTime;
    return *this;
}


Context& Context::setProperty(std::string key, std::string value)
{
    if (key.empty()) return *this;
    if (isReservedContextKey(key)) {
        //define a logging strategy here!
        return *this;
    }

    auto it = std::find_if(_properties.begin(), _properties.end(),
                        [&](const auto& kv){ return kv.first == key; });
    if (it != _properties.end()) it->second = std::move(value);
    else _properties.emplace_back(std::move(key), std::move(value));
    return *this;
}

void Context::resolveSessionId()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    
    std::uniform_int_distribution<unsigned int> dist(1, 1000000000);
    
    unsigned int randomNum = dist(gen);
    _sessionId =  std::to_string(randomNum);
}



}// namespace unleash