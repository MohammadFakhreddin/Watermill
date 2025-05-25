#include "GenerateGame.h"

#include <fstream>
#include <iostream>
#include <json.hpp>

#include "BedrockLog.hpp"

using namespace nlohmann;
using namespace MFA;

GenerateGame::GenerateGame(const std::filesystem::path &json_path)
{
    std::ifstream file(json_path);
    json data = json::parse(file);
    if (data.is_discarded())
    {
        MFA_LOG_ERROR("JSON file is invalid");
        return;
    }

    auto objects = data["objects"];
    parse_objects(nullptr, objects);
    MFA_LOG_DEBUG("Finished parsing file");
}

void GenerateGame::parse_objects(Transform * parent, json objects)
{
    auto to_vec3 = [](json j)
    {
        glm::vec3 vec;
        j.at("x").get_to(vec.x);
        j.at("y").get_to(vec.y);
        j.at("z").get_to(vec.z);
        return vec;
    };

    auto to_vec2 = [](json j)
    {
        glm::vec2 vec;
        j.at("x").get_to(vec.x);
        j.at("y").get_to(vec.y);
        return vec;
    };

    for (auto const &object : objects)
    {
        transforms.emplace_back(std::make_shared<Transform>());
        auto &transform = transforms.back();
        transform->SetLocalPosition(to_vec3(object["position"]));
        transform->SetEulerAngles(glm::radians(to_vec3(object["rotation"])));
        transform->SetLocalScale(to_vec3(object["scale"]));
        transform->name = object["name"].get<std::string>();
        if (object["tag"] != "Untagged")
        {
            transform->tag = object["tag"].get<std::string>();
        }
        if (parent != nullptr)
        {
            transform->SetParent(parent);
        }

        auto sprite_name = object["spriteName"].get<std::string>();
        if (!sprite_name.empty())
        {
            sprites.emplace_back(std::make_shared<Sprite>());
            auto sprite = sprites.back();
            sprite->name = sprite_name;
            for (auto const & uv : object["spriteUVs"])
            {
                sprite->uvs.push_back(to_vec2(uv));
            }
            sprite->spriteMin = to_vec2(object["spriteMin"]);
            sprite->spriteMax = to_vec2(object["spriteMax"]);
            sprite->worldMin = to_vec2(object["worldMin"]);
            sprite->worldMax = to_vec2(object["worldMax"]);
            sprite->flipX = object["flipX"].get<bool>();
            sprite->flipY = object["flipY"].get<bool>();
            sprite->transform_ptr = transform.get();
        }

        if (!object["children"].empty())
        {
            parse_objects(transform.get(), object["children"]);
        }
    }
}
