#pragma once

#include <future>


#include "GenerateGame.h"
#include "IScene.hpp"
#include "SpriteRenderer.hpp"
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

    std::future<GenerateGame> _jsonTask{};

    std::unique_ptr<WebViewContainer> _webViewContainer;
    std::vector<litehtml::element::ptr> _buttons{};
    int _selectedButton = 0;

    Params _params;

    std::vector<std::shared_ptr<MFA::Transform>> _transforms;

    struct Sprite
    {
        MFA::Transform * transform;
        std::unique_ptr<SpriteRenderer::SpriteData> spriteData;
        std::shared_ptr<MFA::RT::GpuTexture> gpuTexture;
    };
    std::vector<std::shared_ptr<Sprite>> _sprites;
    std::vector<std::shared_ptr<SpriteRenderer::CommandBufferData>> _temps;

    std::shared_ptr<SpriteRenderer> _spriteRenderer;

    float _left{};
    float _right{};
    float _bottom{};
    float _top{};
    float _near{};
    float _far{};
    glm::vec3 _mainCameraPosition{};

    // Temporary
    std::optional<GenerateGame> levelContent;
    std::vector<std::tuple<int, std::shared_ptr<MFA::RT::GpuTexture>>> _loadedImages{};

    GumboNode * _timeText = nullptr;

    float _passedTime{};

};
