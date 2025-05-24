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
        InputParams  inputParams,
        Params  menuParams
    );

    void Update(float deltaTime) override;

    void UpdateBuffer(MFA::RT::CommandRecordState &recordState) override;

    void Render(MFA::RT::CommandRecordState &recordState) override;

    void Resize() override;

    void Reload() override;

private:

    void QueryButtons();

    void SetSelectedButton(int index);

    InputParams _inputParams;
    Params _menuParams;

    std::unique_ptr<WebViewContainer> _webViewContainer;
    std::vector<litehtml::element::ptr> _buttons{};
    int _selectedButton = 0;

};