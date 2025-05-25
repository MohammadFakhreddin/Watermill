#include "GameScene.hpp"

#include <utility>

#include "BedrockPath.hpp"
#include "GenerateGame.h"
#include "LogicalDevice.hpp"

using namespace MFA;

//======================================================================================================================

GameScene::GameScene(
    WebViewContainer::Params const & webviewParams,
    Params gameParams
)
    : _params(std::move(gameParams))
{
    auto const * device = MFA::LogicalDevice::Instance;

    auto const htmlPath = MFA::Path::Instance()->Get("ui/game/Game.html");

    litehtml::position clip;
    clip.x = 0;
    clip.y = 0;
    clip.width = device->GetWindowWidth();
    clip.height = device->GetWindowHeight();

    // WebViewContainer::Params webviewParamsOverride {
    //     .solidFillRenderer = webviewParams.solidFillRenderer,
    //     .imageRenderer = webviewParams.imageRenderer,
    //     .borderRenderer = webviewParams.borderRenderer,
    //     .requestBlob = webviewParams.requestBlob,
    //     .requestFont = webviewParams.requestFont,
    //     .requestImage = [webviewParams](char const * path)->std::tuple<std::shared_ptr<MFA::RT::GpuTexture>, glm::vec2>
    //     {
    //         if (std::string(path).find("game\\game") != std::string::npos)
    //         {
    //             return {nullptr, glm::vec2{1.0f, 1.0f}};
    //         }
    //         return webviewParams.requestImage(path);
    //     }
    // };

    _webViewContainer = std::make_unique<WebViewContainer>(htmlPath.c_str(), clip, webviewParams);

    GenerateGame const levelContent(MFA::Path::Instance()->Get("levels/level1.json"));
    _transforms = levelContent.Transforms();
    auto const & sprites = levelContent.Sprites();

    for (auto & sprite : sprites)
    {
        auto const address = Path::Instance()->Get(sprite->name);
        MFA_ASSERT(std::filesystem::exists(address));
        auto [gpuTexture, imageSize] = webviewParams.requestImage(address.c_str());
        MFA_ASSERT(sprite->uvs.size() == 4);

        auto const matrix = sprite->transform_ptr->GlobalTransform();

        auto hW = imageSize.x * 0.5f;
        auto hH = imageSize.y * 0.5f;

        glm::vec3 const topLeftPosition = matrix * glm::vec4{-hW, -hH * 0.5f, 0.0f, 1.0f};
        glm::vec3 const topRightPosition = matrix * glm::vec4{hW, -hH * 0.5f, 0.0f, 1.0f};
        glm::vec3 const bottomLeftPosition = matrix * glm::vec4{-hW, hH * 0.5f, 0.0f, 1.0f};
        glm::vec3 const bottomRightPosition = matrix * glm::vec4{hW, hH * 0.5f, 0.0f, 1.0f};

        ImagePipeline::UV uv0 {sprite->uvs[0].x, sprite->uvs[0].y};
        ImagePipeline::UV uv1 {sprite->uvs[1].x, sprite->uvs[1].y};
        ImagePipeline::UV uv2 {sprite->uvs[2].x, sprite->uvs[2].y};
        ImagePipeline::UV uv3 {sprite->uvs[3].x, sprite->uvs[3].y};

        ImagePipeline::Radius radius0 {};

        auto imageData = webviewParams.imageRenderer->AllocateImageData(
            *gpuTexture,
            topLeftPosition, topRightPosition, bottomLeftPosition, bottomRightPosition,
            radius0, radius0, radius0, radius0,
            uv0, uv1, uv2, uv3
        );

        _sprites.emplace_back(std::make_shared<Sprite>());
        auto & mySprite = _sprites.back();
        mySprite->transform = sprite->transform_ptr;
        mySprite->imageData = std::move(imageData);
    }

    _imageRenderer = webviewParams.imageRenderer;
}

//======================================================================================================================

void GameScene::Update(float deltaTime)
{
    _webViewContainer->Update();
}

//======================================================================================================================

void GameScene::UpdateBuffer(MFA::RT::CommandRecordState &recordState)
{
    _webViewContainer->UpdateBuffer(recordState);
}

//======================================================================================================================

void GameScene::Render(MFA::RT::CommandRecordState &recordState)
{
    ImagePipeline::PushConstants pushConstants {
        .model = glm::identity<glm::mat4>(),
    };
    for (auto & sprite : _sprites)
    {
        _imageRenderer->Draw(recordState, pushConstants, *sprite->imageData);
    }
    _webViewContainer->DisplayPass(recordState);
}

//======================================================================================================================

void GameScene::Resize()
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

void GameScene::Reload()
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

void GameScene::UpdateInputAxis(const glm::vec2 &inputAxis) {}
void GameScene::ButtonA_Changed(bool value) {}
void GameScene::ButtonB_Pressed(bool value)
{
    if (value == true)
    {
        _params.BackPressed();
    }
}

//======================================================================================================================
