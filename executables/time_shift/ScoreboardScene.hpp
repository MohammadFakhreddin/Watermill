#pragma once

#include "IScene.hpp"
#include "WebViewContainer.hpp"

class ScoreboardScene final : public IScene
{
public:

    struct Params
    {
        std::function<void()> BackPressed;
    };

    explicit ScoreboardScene(
        WebViewContainer::Params const & webviewParams,
        Params  params
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

    Params _params;

    std::unique_ptr<WebViewContainer> _webViewContainer;
    std::vector<litehtml::element::ptr> _buttons{};
    int _selectedButton = 0;
};
