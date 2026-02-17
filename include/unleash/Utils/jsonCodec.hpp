#pragma once

#include <string>
#include <optional>
#include <iostream>
#include "unleash/Domain/context.hpp"
#include "unleash/Domain/toggleSet.hpp"
#include <nlohmann/json.hpp>

namespace unleash {

class JsonCodec {
public:
    using json = nlohmann::json;
    

    static ToggleSet decodeClientFeaturesResponse(const std::string& jsonText);

    static std::string encodeContextRequestBody(const Context& ctx);

};

} // namespace unleash