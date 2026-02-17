#include "unleash/Fetcher/toggleFetcher.hpp"
#include "unleash/Utils/utils.hpp"
#include "unleash/Utils/jsonCodec.hpp"

#include <cctype>


namespace unleash
{

ToggleFetcher::ToggleFetcher(const ClientConfig& p_config)
{
    makeFrontendRequest(p_config);
}

void ToggleFetcher::makeFrontendRequest(const ClientConfig& p_config)
{
    _httpRequest.url = p_config.url();
    _httpRequest.usePOSTrequests = p_config.usePostRequests();
    _httpRequest.timeoutMs = static_cast<long>(p_config.timeOutQuery().count());
    // Fill headers similar to JS parseHeaders
    _httpRequest.headers.clear();
    _httpRequest.headers["accept"] = "application/json";
    _httpRequest.headers["'unleash-connection-id'"] = p_config.connectionId();
    // Authorization header (configurable name)
    _httpRequest.headers[utils::keysToLowerCase(p_config.headerName())] = p_config.clientKey();

    _httpRequest.headers["unleash-sdk"] = std::string(utils::sdkVersion);
    _httpRequest.headers["user-agent"] = std::string(utils::agentVersion);
    
    _httpRequest.headers["unleash-appname"] = p_config.appName();

    if (_httpRequest.usePOSTrequests) {
        _httpRequest.headers["content-type"] = "application/json";
    }
    if(!_etag.empty()) _httpRequest.headers["if-none-match"] = _etag;
    // Custom headers (override defaults if same key)
    for (const auto& [k, v] : p_config.customHeaders()) {
        if (k.empty()) continue;
        _httpRequest.headers[utils::keysToLowerCase(k)] = v;
    }
}

ToggleFetcher::FetchResult ToggleFetcher::fetch(const Context& p_ctx)
{
    _httpRequest.body = JsonCodec::encodeContextRequestBody(p_ctx);
    auto resp = _httpClient.request(_httpRequest);
    FetchResult result;

    if (!resp) 
    { 
        result.error = "Error: null response from HttpClient"; return result; 
    }

    //verify if it's an error response: 
    auto httpError = dynamic_cast<ErrorResponse*>(resp.get());   
    if(httpError)
    {
        std::string errorMessage  = "Request failed with code error <" + std::to_string(static_cast<int>(httpError->code())) 
                                    +"> and message:\n "+  httpError->message();

        result.error = std::move(errorMessage);
        return result;
    }
    auto httpResponse = dynamic_cast<HttpResponse*>(resp.get());
    if(!httpResponse)
    {
        std::string errorMessage  = "Error: The resulting response is not of type: HttpResponse.";
        result.error = std::move(errorMessage);

        return result;
    }
    // Transport-level error...
    if (httpResponse->status == 0) 
    {
        result.status = httpResponse->status;        
        std::string errorMessage  = "Transport error (status=0). Network failure / DNS / connection refused / etc.";
        result.error = std::move(errorMessage);
        return result;        
    }
    result.status = httpResponse->status;
    //No modification case: 
    if (httpResponse->status == utils::httpStatusNoUpdate) 
    {
        return result;                
    }

    ToggleSet toggleSet;
    if (httpResponse->status >= utils::httpStatusOkLower && httpResponse->status < utils::httpStatusOkUpper) 
    {
        try
        {
            auto toggleSet = JsonCodec::decodeClientFeaturesResponse(httpResponse->body);
            if(toggleSet.size()) result.toggles = std::move(toggleSet);
        }
        catch (const std::exception& e)
        {
            result.error = std::string("Failed to decode toggles JSON: ") + e.what();
            return result;
        }
        auto it = httpResponse->headers.find("etag");
        if (it != httpResponse->headers.end() && !it->second.empty())
        {
            _etag = it->second;
            _httpRequest.headers["if-none-match"] = _etag;  
        }
        return result;
    }
    result.error = "Error: " + httpResponse->errorMessage;
    return result;  
}

} // namespace unleash
