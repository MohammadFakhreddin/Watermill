#pragma once

#include <future>


#include "GenerateGame.h"
#include "IScene.hpp"
#include "SpriteRenderer.hpp"
#include "ThreadSafeQueue.hpp"
#include "Transform.hpp"
#include "WebViewContainer.hpp"
#include "camera/ArcballCamera.hpp"

class GameScene final : public IScene
{
public:

    struct Params
    {
        std::string levelName{};
        std::string levelPath{};
        std::function<void()> backPressed;
        std::function<void()> nextLevel;
        std::shared_ptr<SpriteRenderer> spriteRenderer;
    };

    explicit GameScene(
        WebViewContainer::Params const &webviewParams,
        Params gameParams
    );

    ~GameScene();

    void Update(float deltaTime) override;

    void UpdateBuffer(MFA::RT::CommandRecordState &recordState) override;

    void Render(MFA::RT::CommandRecordState &recordState) override;

    void Resize() override;

    void Reload() override;

    void UpdateInputAxis(const glm::vec2 &inputAxis) override;

    void ButtonA_Changed(bool value) override;

    void ButtonB_Pressed(bool value) override;

private:

    void ReadLevelFromJson();

    std::future<LevelParser> _jsonTask{};

    std::unique_ptr<WebViewContainer> _webViewContainer;
    std::vector<litehtml::element::ptr> _buttons{};
    int _selectedButton = 0;

    Params _params;

    std::vector<std::shared_ptr<MFA::Transform>> _transforms;

    struct Sprite
    {
        std::shared_ptr<SpriteRenderer::SpriteData> spriteData;
        std::shared_ptr<MFA::RT::GpuTexture> gpuTexture;
    };
    std::vector<std::shared_ptr<Sprite>> _sprites;

    struct SpriteInstance
    {
        Sprite * sprite;
        MFA::Transform * transform;
        glm::mat4 scaleMat;
        glm::vec4 color;
    };
    std::vector<std::shared_ptr<SpriteInstance>> _instances;
    // TODO: Instance rendering

    struct TemporaryMemory
    {
        int lifeTime = 0;
        std::shared_ptr<SpriteRenderer::CommandBufferData> memory;
        std::shared_ptr<MFA::RT::CommandBufferGroup> commandBuffer;
    };
    std::vector<TemporaryMemory> _temporaryMemories;

    std::shared_ptr<SpriteRenderer> _spriteRenderer;

    float _cameraLeft{};
    float _cameraRight{};
    float _cameraBottom{};
    float _cameraTop{};
    float _cameraNear{};
    float _cameraFar{};
    glm::vec3 _mainCameraPosition{};

    // Temporary, We have to remove this object
    std::optional<LevelParser> levelContent;

    GumboNode * _timeText = nullptr;

    float _passedTime{};

    MFA::ThreadSafeQueue<std::function<void(GameScene * scene, MFA::RenderTypes::CommandRecordState &recordState)>> _nextUpdateTasks{};

};
