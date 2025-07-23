#pragma once

#include "BedrockAssert.hpp"
#include "BedrockMemory.hpp"
#include "Transform.hpp"

#include <filesystem>
#include <json.hpp>

class LevelParser {
public:

    enum class ComponentType : int
    {
        Invalid = 0,
        SpriteRenderer = 1,
        Camera = 2,
        BoxCollider2D = 3,
        PatrolEnemy = 4
    };

    struct Component
    {
        ComponentType type = ComponentType::Invalid;
        int index = -1;
    };

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
    using Index = uint16_t;

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

    struct BoxCollider2D
    {
        bool isTrigger {};
        glm::vec2 offset {};
        glm::vec2 size {};

        MFA::Transform * transform = nullptr;
    };

    struct PatrolEnemy
    {
        float movementSpeed {};
        std::vector<glm::vec2> patrolPositions {};

        MFA::Transform * transform = nullptr;
    };

    explicit LevelParser(const std::filesystem::path &json_path);

    [[nodiscard]]
    std::vector<std::shared_ptr<MFA::Transform>> const & GetTransforms() const {return transforms;}

    [[nodiscard]]
    std::vector<std::shared_ptr<SpriteRenderer>> const & GetSpriteInstances(int const spriteIndex)
    {
        MFA_ASSERT(rendererMap.contains(spriteIndex));
        return rendererMap[spriteIndex];
    }

    [[nodiscard]]
    std::vector<std::shared_ptr<Sprite>> const & GetSprites() const {return sprites;}

    [[nodiscard]]
    std::vector<std::shared_ptr<Camera>>  const & GetCameras() const {return cameras;}

    [[nodiscard]]
    std::vector<std::shared_ptr<Buffer>> const & GetBuffers() const {return buffers;}

    [[nodiscard]]
    std::shared_ptr<MFA::Blob> const & GetBlob() const {return blob;}

    [[nodiscard]]
    std::vector<std::shared_ptr<BoxCollider2D>> GetColliders() const {return colliders;}

    [[nodiscard]]
    std::vector<std::shared_ptr<PatrolEnemy>> GetPatrolEnemy() const {return patrolEnemies;}

private:

    void ParseBuffers(nlohmann::json const & rawBuffers);
    void ParseTextures(nlohmann::json const & rawTextures);
    void ParseSprites(nlohmann::json const & rawSprites);
    void ParseCameras(nlohmann::json const & rawCameras);
    void ParseSpriteRenderers(nlohmann::json const & rawSpriteRenderers);
    void ParseTransforms(MFA::Transform * parent, nlohmann::json const & rawChildren);
    void ParseBoxColliders2D(nlohmann::json const & rawBoxColliders);
    void ParsePatrolEnemies(nlohmann::json const & rawPatrolEnemies);

    std::vector<std::shared_ptr<MFA::Transform>> transforms {};

    std::unordered_map<int, std::vector<std::shared_ptr<SpriteRenderer>>> rendererMap {};
    std::vector<std::shared_ptr<SpriteRenderer>> renderers{};

    std::vector<std::shared_ptr<BoxCollider2D>> colliders {};

    std::vector<std::shared_ptr<PatrolEnemy>> patrolEnemies {};

    std::vector<std::shared_ptr<Sprite>> sprites {};
    std::vector<std::shared_ptr<Camera>> cameras {};
    std::vector<std::string> textures {};
    std::vector<std::shared_ptr<Buffer>> buffers {};
    std::shared_ptr<MFA::Blob> blob {};

};