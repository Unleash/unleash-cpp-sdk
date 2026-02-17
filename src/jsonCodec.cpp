#include "unleash/Utils/jsonCodec.hpp"


namespace unleash{

ToggleSet JsonCodec::decodeClientFeaturesResponse(const std::string& jsonText)
{
    json root = json::parse(jsonText, nullptr, false);
    if (root.is_discarded()) {
        std::cerr << "Received string is not a json..." << std::endl;
        return ToggleSet();
    }

    auto itToggles = root.find("toggles");
    if (itToggles == root.end()) {
        std::cout << "Received string doesn't contain \"toggles\" field" << std::endl;
        return ToggleSet();
    }
    if (!itToggles->is_array()) {
        std::cout << "The field \"toggles\" contained in the received string is not an array " << std::endl;
        return ToggleSet();
    }

    const json& toggles = *itToggles;

    std::vector<Toggle> vToggles;
    vToggles.reserve(toggles.size());

    for (const auto& item : toggles)
    {
        if (!item.is_object()) continue;
        const json& obj = item;

        // ---- name ----
        auto itName = obj.find("name");
        if (itName == obj.end() || !itName->is_string()) {
            continue;
        }
        //Update from here: 
        std::string toggleName = itName->get<std::string>();

        auto itEnabled = obj.find("enabled");
        bool toggleEnabled = (itEnabled != obj.end() && itEnabled->is_boolean()) ?  itEnabled->get<bool>() : false;
        
        
        auto itImpr = obj.find("impressionData");
        bool toggleImpression = (itImpr != obj.end() && itImpr->is_boolean()) ? itImpr->get<bool>() : false;

        if(!toggleEnabled)
        {
            vToggles.emplace_back(Toggle{std::move(toggleName), false, toggleImpression});
            continue;
        }
        
        auto itVariant = obj.find("variant");
        if (itVariant == obj.end() || !itVariant->is_object()) {
            vToggles.emplace_back(Toggle{std::move(toggleName), toggleEnabled, toggleImpression});
            continue;
        }

        const json& vObj = *itVariant;

        auto itVName = vObj.find("name");
        auto itVEnabled = vObj.find("enabled");
        if (itVName == vObj.end() || !itVName->is_string() || itVName->get<std::string>().empty() ||
            itVEnabled == vObj.end() || !itVEnabled->is_boolean() || !itVEnabled->get<bool>())
        {
            vToggles.emplace_back(Toggle{std::move(toggleName), toggleEnabled, toggleImpression});
            continue;
        }
        auto itPayload = vObj.find("payload");
        if (itPayload == vObj.end() || !itPayload->is_object())
        {
            vToggles.emplace_back(Toggle{std::move(toggleName), toggleEnabled, toggleImpression, 
                                    Variant{itVName->get<std::string>(), itVEnabled->get<bool>()}});
            continue;
        }
        const json& pObj = *itPayload;
        auto itType  = pObj.find("type");
        auto itValue = pObj.find("value");
        if (itType == pObj.end() || !itType->is_string() || itType->get<std::string>().empty() || itValue == pObj.end() || !itValue->is_string())
        {
            vToggles.emplace_back(Toggle{std::move(toggleName), toggleEnabled, toggleImpression, 
                                    Variant{itVName->get<std::string>(), itVEnabled->get<bool>()}});
            continue;
        }
        vToggles.emplace_back(Toggle{std::move(toggleName), toggleEnabled, toggleImpression, 
                                Variant{itVName->get<std::string>(), itVEnabled->get<bool>(), 
                                Variant::Payload{itType->get<std::string>(), itValue->get<std::string>()}}});

    }

    return ToggleSet(std::move(vToggles));
}

// Domain -> request body
std::string JsonCodec::encodeContextRequestBody(const Context& ctx)
{  
    json j = nlohmann::json::object();
    j["appName"]     = ctx.getAppName();
    j["sessionId"] = ctx.getSessionId();

    if (ctx.getEnvironment())        j["environment"]        = *ctx.getEnvironment();
    if (ctx.getUserId())        j["userId"]        = *ctx.getUserId();
    if (ctx.getRemoteAddress()) j["remoteAddress"] = *ctx.getRemoteAddress();
    if (ctx.getCurrentTime())   j["currentTime"]   = *ctx.getCurrentTime();
    if(!ctx.getProperties().empty())
    {
        json vProperties;
        for (const auto& [key, value] : ctx.getProperties()) vProperties[key] = value;
        j["properties"]  = vProperties;
    }
    

    json root;
    root["context"] = j;
    return root.dump();

}
}// namespace unleash



