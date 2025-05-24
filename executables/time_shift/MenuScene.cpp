#include "MenuScene.hpp"

#include <utility>

#include "BedrockPath.hpp"
#include "LogicalDevice.hpp"

using namespace MFA;

//======================================================================================================================

MenuScene::MenuScene(
    WebViewContainer::Params const & webviewParams,
    InputParams inputParams,
    Params menuParams
)
    : _inputParams(std::move(inputParams))
    , _menuParams(std::move(menuParams))
{
    auto const * device = LogicalDevice::Instance;

    auto const htmlPath = Path::Instance()->Get("ui/menu/Menu.html");

    litehtml::position clip;
    clip.x = 0;
    clip.y = 0;
    clip.width = device->GetWindowWidth();
    clip.height = device->GetWindowHeight();

    _webViewContainer = std::make_unique<WebViewContainer>(htmlPath.c_str(), clip, webviewParams);

    QueryButtons();
}

//======================================================================================================================

float inputAxisCooldown = 0.0f;
void MenuScene::Update(float deltaTime)
{
    _webViewContainer->Update();

    if (inputAxisCooldown <= 0.0f)
    {
        glm::vec2 const inputAxis = _inputParams.InputAxis();
        if (std::abs(inputAxis.y) > 0.0f)
        {
            inputAxisCooldown = 0.25f;
            if (inputAxis.y > 0.0f)
            {
                SetSelectedButton(_selectedButton + 1);
            }
            else
            {
                SetSelectedButton(_selectedButton - 1);
            }
        }
    }
    else
    {
        inputAxisCooldown -= deltaTime;
    }

    if (_inputParams.IsButtonA_Pressed() == true)
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
