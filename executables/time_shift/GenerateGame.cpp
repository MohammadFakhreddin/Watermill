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
        transform->SetEulerAngles((to_vec3(object["rotation"])));
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

        auto const sprite_name = object["spriteName"].get<std::string>();
        if (!sprite_name.empty())
        {
            sprites.emplace_back(std::make_shared<Sprite>());
            auto & sprite = sprites.back();
            sprite->name = sprite_name;
            for (auto const & vertex : object["spriteVertices"])
            {
                sprite->vertices.emplace_back(glm::vec3{to_vec2(vertex), 0.0f});
            }
            for (auto const & uv : object["spriteUVs"])
            {
                sprite->uvs.push_back(to_vec2(uv));
            }
            for (auto const & index : object["spriteTriangles"])
            {
                sprite->indices.emplace_back(index.get<uint16_t>());
            }

            // TODO: Restore support for flip
            sprite->flipX = false;//object["flipX"].get<bool>();
            sprite->flipY = false;//object["flipY"].get<bool>();
            sprite->transform_ptr = transform.get();
        }

        if (!object["children"].empty())
        {
            parse_objects(transform.get(), object["children"]);
        }
    }
}
