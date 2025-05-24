#include "MenuScene.hpp"

#include "BedrockPath.hpp"
#include "LogicalDevice.hpp"

using namespace MFA;

//======================================================================================================================

MenuScene::MenuScene(WebViewContainer::Params & params)
{
    auto const * device = LogicalDevice::Instance;

    auto const htmlPath = Path::Instance()->Get("ui/menu/Menu.html");

    litehtml::position clip;
    clip.x = 0;
    clip.y = 0;
    clip.width = device->GetWindowWidth();
    clip.height = device->GetWindowHeight();

    _webViewContainer = std::make_unique<WebViewContainer>(htmlPath.c_str(), clip, params);

    QueryButtons();

}

//======================================================================================================================

void MenuScene::Update(float deltaTime) { _webViewContainer->Update(); }

//======================================================================================================================

void MenuScene::UpdateBuffer(MFA::RT::CommandRecordState &recordState)
{
    _webViewContainer->UpdateBuffer(recordState);
}

//======================================================================================================================

void MenuScene::Render(MFA::RT::CommandRecordState &recordState)
{
    _webViewContainer->DisplayPass(recordState);
}

//======================================================================================================================

void MenuScene::Resize()
{
    auto const *device = LogicalDevice::Instance;

    litehtml::position clip;
    clip.x = 0;
    clip.y = 0;
    clip.width = device->GetWindowWidth();
    clip.height = device->GetWindowHeight();

    _webViewContainer->OnResize(clip);
}

//======================================================================================================================

void MenuScene::Reload()
{
    auto const * device = LogicalDevice::Instance;

    litehtml::position clip;
    clip.x = 0;
    clip.y = 0;
    clip.width = device->GetWindowWidth();
    clip.height = device->GetWindowHeight();

    _webViewContainer->OnReload(clip);
}

//======================================================================================================================

void MenuScene::QueryButtons()
{
    _buttons.clear();
    {
        auto const element = _webViewContainer->GetElementById("play");
        if (element != nullptr)
        {
            _buttons.emplace_back(element);
        }
    }
    {
        auto const element = _webViewContainer->GetElementById("scoreboard");
        if (element != nullptr)
        {
            _buttons.emplace_back(element);
        }
    }
    SetSelectedButton(0);
}

//======================================================================================================================

void MenuScene::SetSelectedButton(int const index)
{
    _selectedButton = (index + (int)_buttons.size()) % (int)_buttons.size();

    for (const auto & button : _buttons)
    {
        _webViewContainer->RemoveClass(button, "selected");
        _webViewContainer->AddClass(button, "unselected");
    }

    if (_selectedButton >= 0)
    {
        _webViewContainer->RemoveClass(_buttons[_selectedButton], "unselected");
        _webViewContainer->AddClass(_buttons[_selectedButton], "selected");
    }
}

//======================================================================================================================
