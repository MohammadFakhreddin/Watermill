#pragma once

#include "json.hpp"

namespace MFA::JsonUtils
{
    template<typename T>
    static T TryGetValue(
        nlohmann::json const & jsonObject,
        char const * keyName,
        T defaultValue
    )
    {
        T result = defaultValue;
        const auto findResult = jsonObject.find(keyName);
        if (findResult != jsonObject.end())
        {
            result = jsonObject.value<T>(keyName, defaultValue);
        }
        return result;
    }

    template<typename T>
    static std::vector<T> TryGetArray(
        nlohmann::json const & jsonObject,
        std::string const & keyName,
        std::function<T(nlohmann::json const &)> ParseItem = [](nlohmann::json const & j) { return j.get<T>(); }
    )
    {
        std::vector<T> result {};
        const auto findResult = jsonObject.find(keyName);
        if (findResult != jsonObject.end())
        {
            result.clear();
            for (auto & rawItem : jsonObject[keyName])
            {
                result.emplace_back(ParseItem(rawItem));
            }
        }
        return result;
    }
}

namespace MFA
{
    namespace JU = JsonUtils;
}