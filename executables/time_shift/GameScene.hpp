#pragma once

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
        std::function<void()> backPressed;
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

    std::unique_ptr<WebViewContainer> _webViewContainer;
    std::vector<litehtml::element::ptr> _buttons{};
    int _selectedButton = 0;

    Params _params;

    std::vector<std::shared_ptr<MFA::Transform>> _transforms;

    struct Sprite
    {
        MFA::Transform * transform;
        std::unique_ptr<SpriteRenderer::SpriteData> spriteData;
    };
    std::vector<std::shared_ptr<Sprite>> _sprites;
    std::shared_ptr<SpriteRenderer> _spriteRenderer;

    float _left{};
    float _right{};
    float _bottom{};
    float _top{};
    float _near{};
    float _far{};
};
