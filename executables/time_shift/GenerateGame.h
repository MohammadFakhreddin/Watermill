#pragma once

#include "BedrockAssert.hpp"
#include "Transform.hpp"

#include <filesystem>
#include <json.hpp>

// TODO: Rename this file to Level parse after it is working

class LevelParser {
public:

    struct Sprite
    {
        int textureName = -1;
        std::string spriteName {};
        int vertexBufferIndex = -1;
        int indexBufferIndex = -1;
        int uvBufferIndex = -1;
    };

    struct Buffer
    {
        int offset = 0;
        int size = 0;
    };

    struct Camera
    {
        float top;
        float left;
        float right;
        float bottom;
        float near;
        float far;
    };

    struct SpriteInstance
    {
        int spriteIndex;
        bool flipX = false;
        bool flipY = false;
        glm::vec4 color{};
    };

    explicit LevelParser(const std::filesystem::path &json_path);

    [[nodiscard]]
    std::vector<std::shared_ptr<MFA::Transform>> const & GetTransforms() const {return transforms;}

    [[nodiscard]]
    std::vector<std::shared_ptr<SpriteInstance>> const & GetSpriteInstances(int const spriteIndex)
    {
        MFA_ASSERT(spriteInstanceMap.contains(spriteIndex));
        return spriteInstanceMap[spriteIndex];
    }

    [[nodiscard]]
    std::vector<std::shared_ptr<Sprite>> const & GetSprites() const {return sprites;}

    [[nodiscard]]
    std::vector<std::shared_ptr<Camera>>  const & GetCameras() const {return cameras;}

private:

    void ParseTransforms(MFA::Transform * parent, nlohmann::json const & rawChildren);
    void ParseCamera(MFA::Transform * transform, nlohmann::json const & object);
    void ParseSprites(nlohmann::json const & objects);
    void ParseSpriteRenderer(MFA::Transform * transform, nlohmann::json const & object);

    std::vector<std::shared_ptr<MFA::Transform>> transforms;
    std::unordered_map<int, std::vector<std::shared_ptr<SpriteInstance>>> spriteInstanceMap;
    std::vector<std::shared_ptr<Sprite>> sprites;
    std::vector<std::shared_ptr<Camera>> cameras;


};