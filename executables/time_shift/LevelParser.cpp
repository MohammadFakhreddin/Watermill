#include "LevelParser.hpp"

#include "BedrockLog.hpp"
#include "JsonUtils.hpp"
#include "ScopeProfiler.hpp"
#include "BedrockFile.hpp"

#include <fstream>
#include <json.hpp>

using namespace MFA;

static glm::vec3 ToVec3(nlohmann::json const & j)
{
    glm::vec3 vec;
    j.at("x").get_to(vec.x);
    j.at("y").get_to(vec.y);
    j.at("z").get_to(vec.z);
    return vec;
};

static glm::vec2 ToVec2(nlohmann::json const & j)
{
    glm::vec2 vec;
    j.at("x").get_to(vec.x);
    j.at("y").get_to(vec.y);
    return vec;
}

static glm::vec4 ToColor(nlohmann::json const & j)
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
        nlohmann::json data = nlohmann::json::parse(file);
        if (data.is_discarded())
        {
            MFA_LOG_ERROR("JSON file is invalid");
            return;
        }

        ParseBuffers(JU::TryGetValue(data, "buffers", nlohmann::json{}));
        ParseTextures(JU::TryGetValue(data, "textures", nlohmann::json{}));
        ParseSprites(JU::TryGetValue(data, "sprites", nlohmann::json{}));
        ParseCameras(JU::TryGetValue(data, "cameras", nlohmann::json{}));
        ParseSpriteRenderers(JU::TryGetValue(data, "spriteRenderers", nlohmann::json{}));
        ParseBoxColliders2D(JU::TryGetValue(data, "boxColliders", nlohmann::json{}));
        ParsePatrolEnemies(JU::TryGetValue(data, "patrolEnemies", nlohmann::json{}));
        ParseTransforms(nullptr, JU::TryGetValue(data, "transforms", nlohmann::json{}));

        {
#ifdef MFA_DEBUG
            for (auto const & instance : renderers)
            {
                MFA_ASSERT(instance->transform != nullptr);
            }
            for (auto const & camera : cameras)
            {
                MFA_ASSERT(camera->transform != nullptr);
            }
            for (auto const & collider : colliders)
            {
                MFA_ASSERT(collider->transform != nullptr);
            }
            for (auto const & patrolEnemy : patrolEnemies)
            {
                MFA_ASSERT(patrolEnemy->transform != nullptr);
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

void LevelParser::ParseTransforms(Transform *parent, nlohmann::json const &rawChildren)
{
    for (auto const &object : rawChildren)
    {
        transforms.emplace_back(std::make_shared<Transform>());
        auto &transform = transforms.back();
        transform->SetLocalPosition(ToVec3(JU::TryGetValue(object, "position", nlohmann::json{})));
        transform->SetEulerAngles(ToVec3(JU::TryGetValue(object, "rotation", nlohmann::json{})));
        transform->SetLocalScale(ToVec3(JU::TryGetValue(object, "scale", nlohmann::json{})));
        transform->name = JU::TryGetValue<std::string>(object, "name", "");
        transform->tag = JU::TryGetValue<std::string>(object, "tag", "");

        if (parent != nullptr)
        {
            transform->SetParent(parent);
        }

        {// Components
            auto const findResult = object.find("components");
            if (findResult != object.end())
            {
                for (auto & rawComponent : object["components"])
                {
                    auto type = JU::TryGetValue<ComponentType>(
                        rawComponent,
                        "type",
                        ComponentType::Invalid
                    );
                    MFA_ASSERT(type != ComponentType::Invalid);

                    auto const index = JU::TryGetValue<int>(rawComponent, "index", -1);
                    MFA_ASSERT(index >= 0);

                    switch (type)
                    {
                        case ComponentType::SpriteRenderer:
                        {
                            MFA_ASSERT(index < renderers.size());
                            renderers[index]->transform = transform.get();
                            break;
                        }
                        case ComponentType::Camera:
                        {
                            MFA_ASSERT(index < cameras.size());
                            cameras[index]->transform = transform.get();
                            break;
                        }
                        case ComponentType::BoxCollider2D:
                        {
                            MFA_ASSERT(index < colliders.size());
                            colliders[index]->transform = transform.get();
                            break;
                        }
                        case ComponentType::PatrolEnemy:
                        {
                            MFA_ASSERT(index < patrolEnemies.size());
                            patrolEnemies[index]->transform = transform.get();
                            break;
                        }
                        default:
                        {
                            MFA_LOG_ERROR("Unknown component type: {}", static_cast<int>(type));
                            break;
                        }
                    }
                }
            }
        }

        if (object["children"].empty() == false)
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
    renderers.clear();
    rendererMap.clear();
    for (auto const & rawSpriteRenderer : rawSpriteRenderers)
    {
        auto spriteInstance = std::make_shared<SpriteRenderer>();
        spriteInstance->spriteIndex = rawSpriteRenderer["spriteIndex"];
        spriteInstance->flipX = rawSpriteRenderer["flipX"];
        spriteInstance->flipY = rawSpriteRenderer["flipY"];
        spriteInstance->color = ToColor(rawSpriteRenderer["color"]);

        rendererMap[spriteInstance->spriteIndex].emplace_back(spriteInstance);
        renderers.emplace_back(spriteInstance);
    }
}

//======================================================================================================================

void LevelParser::ParseBoxColliders2D(nlohmann::json const &rawBoxColliders)
{
    for (auto const & rawBoxCollider : rawBoxColliders)
    {
        auto boxCollider = std::make_shared<BoxCollider2D>();
        boxCollider->isTrigger = JU::TryGetValue(rawBoxCollider, "isTrigger", false);
        boxCollider->offset = ToVec2(JU::TryGetValue(rawBoxCollider, "offset", nlohmann::json{}));
        boxCollider->size = ToVec2(JU::TryGetValue(rawBoxCollider, "size", nlohmann::json{}));

        colliders.emplace_back(boxCollider);
    }
}

//======================================================================================================================

void LevelParser::ParsePatrolEnemies(nlohmann::json const & rawPatrolEnemies)
{
    for (auto const & rawPatrolEnemy : rawPatrolEnemies)
    {
        auto patrolEnemy = std::make_shared<PatrolEnemy>();
        patrolEnemy->movementSpeed = JU::TryGetValue(rawPatrolEnemy, "movementSpeed", 0.0f);
        patrolEnemy->patrolPositions = {};
        patrolEnemy->patrolPositions = JU::TryGetArray<glm::vec2>(
            rawPatrolEnemy,
            "patrolPositions",
            [](nlohmann::json const & json)
            {
                return ToVec2(json);
            }
        );
        patrolEnemies.emplace_back(patrolEnemy);
    }
}

//======================================================================================================================
