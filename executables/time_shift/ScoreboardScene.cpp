#include "ScoreboardScene.hpp"

#include "BedrockPath.hpp"
#include "LogicalDevice.hpp"

using namespace MFA;

//======================================================================================================================

ScoreboardScene::ScoreboardScene(
    WebViewContainer::Params const & webviewParams,
    Params params
)
    : _params(std::move(params))
{
    auto const * device = LogicalDevice::Instance;

    auto const htmlPath = Path::Instance()->Get("ui/scoreboard/Scoreboard.html");

    litehtml::position clip;
    clip.x = 0;
    clip.y = 0;
    clip.width = device->GetWindowWidth();
    clip.height = device->GetWindowHeight();

    _webViewContainer = std::make_unique<WebViewContainer>(htmlPath.c_str(), clip, webviewParams);
}

//======================================================================================================================

void ScoreboardScene::Update(float deltaTime)
{
    _webViewContainer->Update();
}

//======================================================================================================================

void ScoreboardScene::UpdateBuffer(RT::CommandRecordState &recordState)
{
    _webViewContainer->UpdateBuffer(recordState);
}

//======================================================================================================================

void ScoreboardScene::Render(RT::CommandRecordState &recordState)
{
    _webViewContainer->DisplayPass(recordState);
}

//======================================================================================================================

void ScoreboardScene::Resize()
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

void ScoreboardScene::Reload()
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

void ScoreboardScene::UpdateInputAxis(const glm::vec2 &inputAxis)
{

}

//======================================================================================================================

void ScoreboardScene::ButtonA_Changed(bool value)
{
}

//======================================================================================================================

void ScoreboardScene::ButtonB_Pressed(bool value)
{
    if (value == true)
    {
        _params.BackPressed();
    }
}

//======================================================================================================================
