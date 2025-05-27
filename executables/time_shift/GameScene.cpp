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

    _spriteRenderer = _params.spriteRenderer;
    MFA_ASSERT(_spriteRenderer != nullptr);

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

    std::vector<std::unique_ptr<SpriteRenderer::CommandBufferData>> commandBufferDataList{};

    device->DeviceWaitIdle();
    auto commandBuffer = RB::BeginSingleTimeCommand(device->GetVkDevice(), device->GetGraphicCommandPool());
    for (auto & sprite : sprites)
    {
        auto const address = Path::Instance()->Get("textures/" + sprite->name);
        if(std::filesystem::exists(address))
        {
            auto [gpuTexture, imageSize] = webviewParams.requestImage(address.c_str());

            glm::vec3 flipScale {1.0f, 1.0f, 1.0f};
            if (sprite->flipX == true)
            {
                flipScale.x *= -1.0f;
            }
            if (sprite->flipY == true)
            {
                flipScale.y *= -1.0f;
            }
            auto scaleMat = glm::scale(glm::mat4(1), flipScale);

            std::vector<SpritePipeline::Position> tVs(sprite->vertices.size());
            std::vector<SpriteRenderer::UV> tUs(sprite->vertices.size());
            for (int i = 0; i < sprite->vertices.size(); ++i)
            {
                auto & oV = sprite->vertices[i];
                auto & tV = tVs[i];

                tV = sprite->transform_ptr->GlobalTransform() * scaleMat * glm::vec4{oV, 1.0f};

                left = std::min(left, tV.x);
                right = std::max(right, tV.x);
                top = std::min(top, tV.y);
                bottom = std::max(bottom, tV.y);
                front = std::min(front, tV.z);
                back = std::max(back, tV.z);

                tUs[i].x = sprite->uvs[i].x;
                tUs[i].y = 1.0f - sprite->uvs[i].y;
            }

            ImagePipeline::Radius radius0 {};

            auto [imageData, tempData] = _spriteRenderer->Allocate(
                commandBuffer,
                *gpuTexture,
                (int)sprite->vertices.size(),
                tVs.data(),
                tUs.data(),
                (int)sprite->indices.size(),
                sprite->indices.data()
            );

            std::shared_ptr mySprite = std::make_shared<Sprite>();
            mySprite->transform = sprite->transform_ptr;
            mySprite->spriteData = std::move(imageData);
            _sprites.emplace_back(mySprite);

            commandBufferDataList.emplace_back(std::move(tempData));
        }
        else
        {
            MFA_LOG_WARN("Failed to find the asset with name: %s", address.c_str());
        }
    }
    RB::EndAndSubmitSingleTimeCommand(
        device->GetVkDevice(),
        device->GetGraphicCommandPool(),
        device->GetGraphicQueue(),
        commandBuffer
    );

    _left = left;
    _right = right;
    _bottom = bottom;
    _top = top;
    _near = back;
    _far = front;
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
    SpritePipeline::PushConstants pushConstants {
        .model = glm::transpose(viewProjectionMatrix)
    };
    for (auto & sprite : _sprites)
    {
        _spriteRenderer->Draw(recordState, pushConstants, *sprite->spriteData);
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
        _params.backPressed();
    }
}

//======================================================================================================================
