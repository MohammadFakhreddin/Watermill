#include "GameScene.hpp"

#include <iostream>
#include <utility>

#include "BedrockMath.hpp"
#include "BedrockPath.hpp"
#include "GenerateGame.h"
#include "LogicalDevice.hpp"
#include "camera/ArcballCamera.hpp"
#include "camera/ObserverCamera.hpp"

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

    _webViewContainer = std::make_unique<WebViewContainer>(htmlPath.c_str(), clip, webviewParams);

    GenerateGame const levelContent(MFA::Path::Instance()->Get("levels/level1.json"));
    _transforms = levelContent.Transforms();
    auto const & sprites = levelContent.Sprites();

    float left = std::numeric_limits<float>::max();
    float right = std::numeric_limits<float>::min();
    float top = std::numeric_limits<float>::max();
    float bottom = std::numeric_limits<float>::min();
    float back = std::numeric_limits<float>::max();
    float front = std::numeric_limits<float>::min();

    for (auto & sprite : sprites)
    {
        auto const address = Path::Instance()->Get("textures/" + sprite->name);
        if(std::filesystem::exists(address))
        {
            auto [gpuTexture, imageSize] = webviewParams.requestImage(address.c_str());

            auto & spriteMin = sprite->spriteMin;
            auto & spriteMax = sprite->spriteMax;

            float uvLeft = spriteMin.x / imageSize.x;
            float uvRight = spriteMax.x / imageSize.x;
            float uvTop = 1.0f - (spriteMin.y / imageSize.y);
            float uvBottom = 1.0f - (spriteMax.y / imageSize.y);

            if (sprite->flipX == true)
            {
                std::swap(uvLeft, uvRight);
            }
            if (sprite->flipY == true)
            {
                std::swap(uvTop, uvBottom);
            }

            ImagePipeline::UV topLeftUV {uvLeft, uvTop};
            ImagePipeline::UV topRightUV {uvRight, uvTop};
            ImagePipeline::UV bottomLeftUV {uvLeft, uvBottom};
            ImagePipeline::UV bottomRightUV {uvRight, uvBottom};

            // auto const uvCenter = glm::vec2(uvLeft + uvRight, uvTop + uvBottom) * 0.5f;
            // auto const centerMat = glm::translate(glm::mat4(1), glm::vec3{uvCenter.x, uvCenter.y, 0.0f});
            // auto const centerInvMat = glm::translate(glm::mat4(1), glm::vec3{-uvCenter.x, -uvCenter.y, 0.0f});

            // auto const eulerAngles =  sprite->transform_ptr->GetLocalRotation().GetEulerAngles();
            // auto rotationMat = glm::toMat4(glm::angleAxis(eulerAngles.z, glm::vec3(0, 0, 1)));

            // topLeftUV = centerMat * rotationMat * centerInvMat * glm::vec4{topLeftUV, 0.0f, 1.0f};
            // topRightUV = centerMat * rotationMat * centerInvMat * glm::vec4{topRightUV, 0.0f, 1.0f};
            // bottomLeftUV = centerMat * rotationMat * centerInvMat * glm::vec4{bottomLeftUV, 0.0f, 1.0f};
            // bottomRightUV = centerMat * rotationMat * centerInvMat * glm::vec4{bottomRightUV, 0.0f, 1.0f};

            auto & worldMin = sprite->worldMin;
            auto & worldMax = sprite->worldMax;

            float worldLeft = worldMin.x;
            float worldRight = worldMax.x;
            float worldTop = worldMin.y;
            float worldBottom = worldMax.y;

            glm::vec4 topLeftPosition = glm::vec4{worldLeft, worldTop, 0.0f, 1.0f};
            glm::vec4 topRightPosition = glm::vec4{worldRight, worldTop, 0.0f, 1.0f};
            glm::vec4 bottomLeftPosition = glm::vec4{worldLeft, worldBottom, 0.0f, 1.0f};
            glm::vec4 bottomRightPosition = glm::vec4{worldRight, worldBottom, 0.0f, 1.0f};

            glm::vec2 const worldCenter ((worldLeft + worldRight) * 0.5f, (worldTop + worldBottom) * 0.5f);
            auto const centerMat = glm::translate(glm::mat4(1), glm::vec3{worldCenter.x, worldCenter.y, 0.0f});
            auto const centerInvMat = glm::translate(glm::mat4(1), glm::vec3{-worldCenter.x, -worldCenter.y, 0.0f});
            auto const eulerAngles =  sprite->transform_ptr->GetLocalRotation().GetEulerAngles();
            auto rotationMat = glm::toMat4(glm::angleAxis(eulerAngles.z, glm::vec3(0, 0, 1)));

            topLeftPosition = centerMat * rotationMat * centerInvMat * topLeftPosition;
            topRightPosition = centerMat * rotationMat * centerInvMat * topRightPosition;
            bottomLeftPosition = centerMat * rotationMat * centerInvMat * bottomLeftPosition;
            bottomRightPosition = centerMat * rotationMat * centerInvMat * bottomRightPosition;

            ImagePipeline::Radius radius0 {};

            std::shared_ptr imageData = webviewParams.imageRenderer->AllocateImageData(
                *gpuTexture,
                topLeftPosition, bottomLeftPosition, topRightPosition, bottomRightPosition,
                radius0, radius0, radius0, radius0,
                topLeftUV, bottomLeftUV, topRightUV, bottomRightUV
            );

            std::shared_ptr mySprite = std::make_shared<Sprite>();
            mySprite->transform = sprite->transform_ptr;
            mySprite->imageData = imageData;
            _sprites.emplace_back(mySprite);

            left = std::min(left, bottomLeftPosition.x);
            right = std::max(right, topRightPosition.x);
            top = std::min(top, topLeftPosition.y);
            bottom = std::max(bottom, bottomLeftPosition.y);
            front = std::max(front, bottomLeftPosition.z);
            back = std::min(back, bottomRightPosition.z);
        }
        else
        {
            MFA_LOG_WARN("Failed to find the asset with name: %s", address.c_str());
        }
    }

    _left = left;
    _right = right;
    _bottom = bottom;
    _top = top;
    _near = back;
    _far = front;

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
    for (auto & sprite : _sprites)
    {
        sprite->imageData->vertexData->Update(recordState);
    }
}

//======================================================================================================================

void GameScene::Render(MFA::RT::CommandRecordState &recordState)
{
    auto worldWidth = (_right - _left);
    auto worldHeight = (_bottom - _top);
    auto yCenter = (worldHeight / 2) + _top;

    auto * device = LogicalDevice::Instance;
    auto windowWidth = device->GetWindowWidth();
    auto windowHeight = device->GetWindowHeight();
    auto ratio = (float)windowWidth / (float)windowHeight;

    auto newHeight = worldWidth / ratio;
    auto bottom = yCenter + newHeight / 2.0f;
    auto top = yCenter - newHeight / 2.0f;

    auto viewProjectionMatrix = glm::ortho(_left, _right, bottom, top, -10.0f + _near, 10.0f + _far);
    ImagePipeline::PushConstants pushConstants {
        .model = glm::transpose(viewProjectionMatrix)
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

    // _camera->SetProjectionDirty();
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
