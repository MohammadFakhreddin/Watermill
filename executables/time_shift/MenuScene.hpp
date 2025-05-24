#pragma once

#include "IScene.hpp"
#include "WebViewContainer.hpp"

class MenuScene final : public IScene
{
public:

    explicit MenuScene(WebViewContainer::Params & params);

    void Update(float deltaTime) override;

    void UpdateBuffer(MFA::RT::CommandRecordState &recordState) override;

    void Render(MFA::RT::CommandRecordState &recordState) override;

    void Resize() override;

    void Reload() override;

private:

    void QueryButtons();

    void SetSelectedButton(int index);

    std::unique_ptr<WebViewContainer> _webViewContainer;
    std::vector<litehtml::element::ptr> _buttons{};
    int _selectedButton = 0;

};