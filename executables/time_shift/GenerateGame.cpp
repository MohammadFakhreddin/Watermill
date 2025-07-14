#include "GenerateGame.h"

#include "BedrockLog.hpp"
#include "JsonUtils.hpp"
#include "ScopeProfiler.hpp"
#include "BedrockFile.hpp"

#include <fstream>
#include <json.hpp>

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

//======================================================================================================================

LevelParser::LevelParser(const std::filesystem::path &json_path)
{
    try
    {
        // MFA_SCOPE_Profiler("Level parser")
        MFA_ASSERT(std::filesystem::exists(json_path));
        // MFA_LOG_INFO("Started parsing file");

        std::ifstream file(json_path);
        json data = json::parse(file);
        if (data.is_discarded())
        {
            MFA_LOG_ERROR("JSON file is invalid");
            return;
        }

        ParseBuffers(JU::TryGetValue(data, "buffers", json{}));
        ParseTextures(JU::TryGetValue(data, "textures", json{}));
        ParseSprites(JU::TryGetValue(data, "sprites", json{}));
        ParseCameras(JU::TryGetValue(data, "cameras", json{}));
        ParseSpriteRenderers(JU::TryGetValue(data, "spriteRenderers", json{}));
        ParseTransforms(nullptr, JU::TryGetValue(data, "transforms", json{}));

        {
#ifdef MFA_DEBUG
            for (auto const & instance : instances)
            {
                MFA_ASSERT(instance->transform != nullptr);
            }
            for (auto const & camera : cameras)
            {
                MFA_ASSERT(camera->transform != nullptr);
            }
#endif
        }

        std::filesystem::path bin_path = json_path;
        bin_path = bin_path.replace_extension(".bin");
        blob = File::Read(bin_path.string());
    }
    catch (std::exception &e)
    {
        MFA_LOG_ERROR(e.what());
    }
}

//======================================================================================================================

void LevelParser::ParseBuffers(nlohmann::json const &rawBuffers)
{
    for (auto const &rawBuffer : rawBuffers)
    {
        buffers.emplace_back(std::make_shared<Buffer>());
        auto const &buffer = buffers.back();
        buffer->type = JU::TryGetValue<BufferType>(rawBuffer, "type", BufferType::Invalid);
        MFA_ASSERT(buffer->type != BufferType::Invalid);
        buffer->offset = JU::TryGetValue<int>(rawBuffer, "offset", 0);
        buffer->size = JU::TryGetValue<int>(rawBuffer, "size", 0);
    }
}

//======================================================================================================================

void LevelParser::ParseTextures(nlohmann::json const &rawTextures)
{
    textures = rawTextures.get<std::vector<std::string>>();
}

//======================================================================================================================

void LevelParser::ParseTransforms(Transform *parent, json const &rawChildren)
{
    for (auto const &object : rawChildren)
    {
        transforms.emplace_back(std::make_shared<Transform>());
        auto &transform = transforms.back();
        transform->SetLocalPosition(ToVec3(JU::TryGetValue(object, "position", json{})));
        transform->SetEulerAngles(ToVec3(JU::TryGetValue(object, "rotation", json{})));
        transform->SetLocalScale(ToVec3(JU::TryGetValue(object, "scale", json{})));
        transform->name = JU::TryGetValue<std::string>(object, "name", "");
        transform->tag = JU::TryGetValue<std::string>(object, "tag", "");

        if (parent != nullptr)
        {
            transform->SetParent(parent);
        }

        int rendererIndex = JU::TryGetValue<int>(object, "spriteRenderer", -1);
        if (rendererIndex >= 0)
        {
            instances[rendererIndex]->transform = transform.get();
        }

        int cameraIndex = JU::TryGetValue<int>(object, "camera", -1);
        if (cameraIndex >= 0)
        {
            cameras[cameraIndex]->transform = transform.get();
        }

        if (!object["children"].empty())
        {
            ParseTransforms(transform.get(), object["children"]);
        }
    }
}

//======================================================================================================================

void LevelParser::ParseSprites(nlohmann::json const &rawSprites)
{
    for (auto const & rawSprite : rawSprites)
    {
        sprites.emplace_back(std::make_shared<Sprite>());
        auto const & sprite = sprites.back();

        int const textureIndex = JU::TryGetValue<int>(rawSprite, "textureName", -1);
        MFA_ASSERT(textureIndex >= 0  && textureIndex < textures.size());
        sprite->textureName = textures[textureIndex];
        sprite->spriteName = JU::TryGetValue<std::string>(rawSprite, "spriteName", "");
        sprite->vertexBufferIndex = JU::TryGetValue(rawSprite, "vertexBufferIndex", -1);
        sprite->indexBufferIndex = JU::TryGetValue(rawSprite, "triangleBufferIndex", -1);
    }
}

//======================================================================================================================

void LevelParser::ParseCameras(nlohmann::json const & rawCameras)
{
    for (auto const & rawCamera : rawCameras)
    {
        cameras.emplace_back(std::make_shared<Camera>());
        auto const & camera = cameras.back();
        camera->top = JU::TryGetValue(rawCamera, "top", 0.0f);
        camera->left = JU::TryGetValue(rawCamera, "left", 0.0f);
        camera->right = JU::TryGetValue(rawCamera, "right", 0.0f);
        camera->bottom = JU::TryGetValue(rawCamera, "bottom", 0.0f);
        camera->near = JU::TryGetValue(rawCamera, "near", 0.0f);
        camera->far = JU::TryGetValue(rawCamera, "far", 0.0f);
    }
}

//======================================================================================================================

void LevelParser::ParseSpriteRenderers(nlohmann::json const & rawSpriteRenderers)
{
    for (auto const & rawSpriteRenderer : rawSpriteRenderers)
    {
        auto spriteInstance = std::make_shared<SpriteRenderer>();
        spriteInstance->spriteIndex = rawSpriteRenderer["spriteIndex"];
        spriteInstance->flipX = rawSpriteRenderer["flipX"];
        spriteInstance->flipY = rawSpriteRenderer["flipY"];
        spriteInstance->color = ToColor(rawSpriteRenderer["color"]);

        instanceMap[spriteInstance->spriteIndex].emplace_back(spriteInstance);
        instances.emplace_back(spriteInstance);
    }

}

//======================================================================================================================
