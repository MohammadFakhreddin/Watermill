#pragma once

#include "IScene.hpp"
#include "WebViewContainer.hpp"

class MenuScene final : public IScene
{
public:

    struct Params
    {
        std::function<void()> PlayPressed;
        std::function<void()> ScoreBoardPressed;
    };

    explicit MenuScene(
        WebViewContainer::Params const & webviewParams,
        Params  menuParams
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

    void QueryButtons();

    void SetSelectedButton(int index);

private:
    Params _menuParams;

    std::unique_ptr<WebViewContainer> _webViewContainer;
    std::vector<GumboNode *> _buttons{};
    int _selectedButton = 0;

};