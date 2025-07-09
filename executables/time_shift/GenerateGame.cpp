#include "GenerateGame.h"

#include <fstream>
#include <json.hpp>

#include "BedrockLog.hpp"
#include "ScopeProfiler.hpp"

using namespace nlohmann;
using namespace MFA;

static glm::vec3 ToVec3(json const & j)
{
    glm::vec3 vec;
    j.at("x").get_to(vec.x);
    j.at("y").get_to(vec.y);
    j.at("z").get_to(vec.z);
    return vec;
};

static glm::vec2 ToVec2(json const & j)
{
    glm::vec2 vec;
    j.at("x").get_to(vec.x);
    j.at("y").get_to(vec.y);
    return vec;
}

static glm::vec4 ToColor(json const & j)
{
    glm::vec4 color;
    j.at("r").get_to(color.x);
    j.at("g").get_to(color.y);
    j.at("b").get_to(color.z);
    j.at("a").get_to(color.w);
    return color;
};

LevelParser::LevelParser(const std::filesystem::path &json_path)
{
    try
    {
        MFA_SCOPE_Profiler("Level parser")
        // MFA_LOG_INFO("Started parsing file");

        std::ifstream file(json_path);
        json data = json::parse(file);
        if (data.is_discarded())
        {
            MFA_LOG_ERROR("JSON file is invalid");
            return;
        }

        ParseSprites(data["sprites"]);
        ParseTransforms(nullptr, data["transforms"]);

        // MFA_LOG_INFO("Finished parsing file");

    } catch (std::exception &e)
    {
        MFA_LOG_ERROR(e.what());
    }
}

void LevelParser::ParseTransforms(Transform *parent, json const & rawChildren)
{
    for (auto const &object : rawChildren)
    {
        transforms.emplace_back(std::make_shared<Transform>());
        auto &transform = transforms.back();
        transform->SetLocalPosition(ToVec3(object["position"]));
        transform->SetEulerAngles((ToVec3(object["rotation"])));
        transform->SetLocalScale(ToVec3(object["scale"]));
        transform->name = object["name"].get<std::string>();
        if (object["tag"] != "Untagged")
        {
            transform->tag = object["tag"].get<std::string>();
        }
        if (parent != nullptr)
        {
            transform->SetParent(parent);
        }

        {
            auto const findResult = object.find("spriteRenderer");
            if (findResult != object.end())
            {
                ParseSpriteRenderer(transform.get(), *findResult);
            }
        }
        {
            auto const findResult = object.find("camera");
            if (findResult != object.end())
            {
                ParseCamera(transform.get(), *findResult);
            }
        }

        if (!object["children"].empty())
        {
            ParseTransforms(transform.get(), object["children"]);
        }
    }
}

void LevelParser::ParseCamera(MFA::Transform * transform, nlohmann::json const & object)
{
    auto isValid = object["isValid"].get<bool>();
    if (isValid == false)
    {
        return;
    }
    cameras.emplace_back(std::make_shared<Camera>());
    auto const & camera = cameras.back();
    camera->top = object["top"].get<float>();
    camera->left = object["left"].get<float>();
    camera->right = object["right"].get<float>();
    camera->bottom = object["bottom"].get<float>();
    camera->near = object["near"].get<float>();
    camera->far = object["far"].get<float>();
    camera->transform = transform;
}

void LevelParser::ParseSprites(nlohmann::json const &objects)
{
    for (auto const & object : objects)
    {
        sprites.emplace_back(std::make_shared<Sprite>());
        auto const & sprite = sprites.back();
        sprite->textureName = object["textureName"];
        sprite->spriteName = object["spriteName"];
        MFA_ASSERT(object.find("vertices") != object.end());
        for (auto const &vertex : object["vertices"])
        {
            sprite->vertices.emplace_back(ToVec2(vertex), 0.0f);
        }
        MFA_ASSERT(object.find("uvs") != object.end());
        for (auto const &uv : object["uvs"])
        {
            sprite->uvs.push_back(ToVec2(uv));
        }
        MFA_ASSERT(object.find("triangles") != object.end());
        for (auto const &index : object["triangles"])
        {
            sprite->indices.emplace_back(index.get<uint16_t>());
        }
    }
}

void LevelParser::ParseSpriteRenderer(Transform * transform, json const & object)
{
    // TODO: Import Json utils into this project
    bool const isValid = object["isValid"].get<bool>();
    if (isValid == false)
    {
        return;
    }
    auto spriteInstance = std::make_shared<SpriteInstance>();
    spriteInstance->transform = transform;
    spriteInstance->spriteIndex = object["spriteIndex"];
    spriteInstance->flipX = object["flipX"];
    spriteInstance->flipY = object["flipY"];
    spriteInstance->color = ToColor(object["color"]);

    spriteInstanceMap[spriteInstance->spriteIndex].emplace_back(spriteInstance);
}
