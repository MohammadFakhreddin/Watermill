#include "MenuScene.hpp"

#include <gumbo.h>
#include <utility>

#include "BedrockPath.hpp"
#include "LogicalDevice.hpp"
#include "Time.hpp"
#include "litehtml/el_text.h"

using namespace MFA;

//======================================================================================================================

MenuScene::MenuScene(
    WebViewContainer::Params const & webviewParams,
    Params menuParams
)
    : _menuParams(std::move(menuParams))
{
    auto const * device = LogicalDevice::Instance;

    auto const htmlPath = Path::Instance()->Get("ui/menu/Menu.html");

    litehtml::position clip;
    clip.x = 0;
    clip.y = 0;
    clip.width = device->GetWindowWidth();
    clip.height = device->GetWindowHeight();

    _webViewContainer = std::make_unique<WebViewContainer>(htmlPath.c_str(), clip, webviewParams);
    _webViewContainer->SetText(_webViewContainer->GetElementById("title"), "The tragedy of fallen game engine! 2");

    QueryButtons();
}

//======================================================================================================================

void MenuScene::Update(float deltaTime)
{
    _webViewContainer->Update();
}

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
    auto const *device = LogicalDevice::Instance;

    litehtml::position clip;
    clip.x = 0;
    clip.y = 0;
    clip.width = device->GetWindowWidth();
    clip.height = device->GetWindowHeight();

    _webViewContainer->OnReload(clip);
}

//======================================================================================================================

float cooldownEndTime = 0.0f;
void MenuScene::UpdateInputAxis(const glm::vec2 &inputAxis)
{
    auto const now = Time::NowSec();
    if (std::abs(inputAxis.y) > 0.0f && now > cooldownEndTime)
    {
        if (inputAxis.y > 0.0f)
        {
            SetSelectedButton(_selectedButton + 1);
        }
        else
        {
            SetSelectedButton(_selectedButton - 1);
        }
        cooldownEndTime = Time::NowSec() + 0.25f;
    }
}

void MenuScene::ButtonA_Changed(bool value)
{
    if (value == true)
    {
        if (_selectedButton == 0)
        {
            _menuParams.PlayPressed();
        }
        else if (_selectedButton == 1)
        {
            _menuParams.ScoreBoardPressed();
        }
    }
}

void MenuScene::ButtonB_Pressed(bool value)
{

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
