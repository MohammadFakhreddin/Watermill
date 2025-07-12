#pragma once

#include "BedrockAssert.hpp"
#include "BedrockMemory.hpp"
#include "Transform.hpp"

#include <filesystem>
#include <json.hpp>

// TODO: Rename this file to Level parse after it is working

class LevelParser {
public:

    struct Sprite
    {
        std::string textureName {};
        std::string spriteName {};
        int vertexBufferIndex = -1;
        int indexBufferIndex = -1;
    };

    struct Vertex
    {
        glm::vec2 position {};
        glm::vec2 uv {};
    };
    using Index = ushort;

    enum class BufferType : int
    {
        Invalid = -1,
        VertexBuffer = 0,
        IndexBuffer = 1,
        Count
    };

    struct Buffer
    {
        BufferType type = BufferType::Invalid;
        int offset = 0;
        int size = 0;
    };

    struct Camera
    {
        float top {};
        float left {};
        float right {};
        float bottom {};
        float near {};
        float far {};

        MFA::Transform * transform = nullptr;
    };

    struct SpriteRenderer
    {
        int spriteIndex;
        bool flipX = false;
        bool flipY = false;
        glm::vec4 color {};

        MFA::Transform * transform = nullptr;
    };

    explicit LevelParser(const std::filesystem::path &json_path);

    [[nodiscard]]
    std::vector<std::shared_ptr<MFA::Transform>> const & GetTransforms() const {return transforms;}

    [[nodiscard]]
    std::vector<std::shared_ptr<SpriteRenderer>> const & GetSpriteInstances(int const spriteIndex)
    {
        MFA_ASSERT(spriteInstanceMap.contains(spriteIndex));
        return instanceMap[spriteIndex];
    }

    [[nodiscard]]
    std::vector<std::shared_ptr<Sprite>> const & GetSprites() const {return sprites;}

    [[nodiscard]]
    std::vector<std::shared_ptr<Camera>>  const & GetCameras() const {return cameras;}

    [[nodiscard]]
    std::vector<std::shared_ptr<Buffer>> const & GetBuffers() const {return buffers;}

    std::shared_ptr<MFA::Blob> const & GetBlob() const {return blob;}

private:

    void ParseBuffers(nlohmann::json const & rawBuffers);
    void ParseTextures(nlohmann::json const & rawTextures);
    void ParseSprites(nlohmann::json const & rawSprites);
    void ParseCameras(nlohmann::json const & rawCameras);
    void ParseSpriteRenderers(nlohmann::json const & rawSpriteRenderers);
    void ParseTransforms(MFA::Transform * parent, nlohmann::json const & rawChildren);

    std::vector<std::shared_ptr<MFA::Transform>> transforms {};
    std::unordered_map<int, std::vector<std::shared_ptr<SpriteRenderer>>> instanceMap {};
    std::vector<std::shared_ptr<SpriteRenderer>> instances{};
    std::vector<std::shared_ptr<Sprite>> sprites {};
    std::vector<std::shared_ptr<Camera>> cameras {};
    std::vector<std::string> textures {};
    std::vector<std::shared_ptr<Buffer>> buffers {};
    std::shared_ptr<MFA::Blob> blob {};

};