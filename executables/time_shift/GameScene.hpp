#pragma once

#include "IScene.hpp"
#include "WebViewContainer.hpp"

class GameScene final : public IScene
{
public:

    struct Params
    {

    };

    explicit GameScene(
        WebViewContainer::Params const &webviewParams,
        InputParams inputParams,
        Params gameParams
    );

    void Update(float deltaTime) override;

    void UpdateBuffer(MFA::RT::CommandRecordState &recordState) override;

    void Render(MFA::RT::CommandRecordState &recordState) override;

    void Resize() override;

    void Reload() override;

private:

    std::unique_ptr<WebViewContainer> _webViewContainer;
    std::vector<litehtml::element::ptr> _buttons{};
    int _selectedButton = 0;

    InputParams _inputParams;
    Params _gameParams;

};
