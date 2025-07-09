#pragma once

#include "GenerateGame.h"
#include "IScene.hpp"
#include "SpriteRenderer.hpp"
#include "Transform.hpp"
#include "WebViewContainer.hpp"
#include "camera/ArcballCamera.hpp"

#include <future>

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

    ~GameScene() override;

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

};
