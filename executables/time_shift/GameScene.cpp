#include "GameScene.hpp"

#include "BedrockMath.hpp"
#include "BedrockPath.hpp"
#include "GenerateGame.h"
#include "LogicalDevice.hpp"
#include "camera/ObserverCamera.hpp"
#include "ResourceManager.hpp"

#include <iostream>
#include <utility>

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

    _webViewContainer->SetText(_webViewContainer->GetElementById("level"), _params.levelName.c_str());
    _webViewContainer->SetText(_webViewContainer->GetElementById("score"), "Score: 0");
    // TODO: Update level text and score dynamically

    auto levelPath = MFA::Path::Instance()->Get(_params.levelPath);
    MFA_ASSERT(std::filesystem::exists(levelPath) == true);

    levelContent = GenerateGame(levelPath);
    _transforms = levelContent->Transforms();
    auto const & sprites = levelContent->Sprites();

    // std::vector<std::unique_ptr<SpriteRenderer::CommandBufferData>> commandBufferDataList{};

    // device->DeviceWaitIdle();
    // auto commandBuffer = RB::BeginSingleTimeCommand(device->GetVkDevice(), device->GetGraphicCommandPool());
    // We can do bindless image loading instead, Also we don't need all the images right away.
    // TODO: We can sort images based on size and load the largest ones last as well
    for (int i = 0; i < (int)sprites.size(); ++i)
    {
        auto const address = Path::Instance()->Get("textures/" + sprites[i]->name);
        if(std::filesystem::exists(address))
        {
            std::string name = "textures/" + sprites[i]->name;
            auto imageFuture = ResourceManager::Instance()->RequestImage(name.c_str());
            _imageFutures.emplace_back(i, std::move(imageFuture));
            // auto [gpuTexture, imageSize] = webviewParams.requestImage(address.c_str());
            //
            // glm::vec3 flipScale {1.0f, 1.0f, 1.0f};
            // if (sprite->flipX == true)
            // {
            //     flipScale.x *= -1.0f;
            // }
            // if (sprite->flipY == true)
            // {
            //     flipScale.y *= -1.0f;
            // }
            // auto scaleMat = glm::scale(glm::mat4(1), flipScale);
            //
            // std::vector<SpritePipeline::Position> tVs(sprite->vertices.size());
            // std::vector<SpriteRenderer::UV> tUs(sprite->vertices.size());
            // for (int i = 0; i < sprite->vertices.size(); ++i)
            // {
            //     auto & oV = sprite->vertices[i];
            //     auto & tV = tVs[i];
            //
            //     tV = sprite->transform_ptr->GlobalTransform() * scaleMat * glm::vec4{oV, 1.0f};
            //
            //     tUs[i].x = sprite->uvs[i].x;
            //     tUs[i].y = 1.0f - sprite->uvs[i].y;
            // }
            //
            // auto [imageData, tempData] = _spriteRenderer->Allocate(
            //     commandBuffer,
            //     *gpuTexture,
            //     (int)sprite->vertices.size(),
            //     tVs.data(),
            //     tUs.data(),
            //     sprite->color,
            //     (int)sprite->indices.size(),
            //     sprite->indices.data()
            // );
            //
            // std::shared_ptr mySprite = std::make_shared<Sprite>();
            // mySprite->transform = sprite->transform_ptr;
            // mySprite->spriteData = std::move(imageData);
            // _sprites.emplace_back(mySprite);

            // commandBufferDataList.emplace_back(std::move(tempData));
        }
        else
        {
            MFA_LOG_WARN("Failed to find the asset with name: %s", address.c_str());
        }
    }
    // RB::EndAndSubmitSingleTimeCommand(
    //     device->GetVkDevice(),
    //     device->GetGraphicCommandPool(),
    //     device->GetGraphicQueue(),
    //     commandBuffer
    // );

    auto const & cameras = levelContent->Cameras();
    if (cameras.empty() == false)
    {
        if (cameras.size() > 1)
        {
            MFA_LOG_WARN("More than one camera detected");
        }

        auto & mainCamera = cameras[0];

        _left = mainCamera->cameraLeft;
        _right = mainCamera->cameraRight;
        auto xCenter = (_left + _right) / 2.0f;
        _left -= xCenter;
        _right -= xCenter;

        _bottom = mainCamera->cameraBottom;
        _top = mainCamera->cameraTop;
        auto yCenter = (_bottom + _top) / 2.0f;
        _bottom -= yCenter;
        _top -= yCenter;

        _near = mainCamera->cameraNear;
        _far = mainCamera->cameraFar;

        _mainCameraPosition = mainCamera->transform_ptr->GlobalPosition();
    }

    // std::sort(_sprites.begin(), _sprites.end(), [](const std::shared_ptr<Sprite>& a, const std::shared_ptr<Sprite>& b) {
    //     return a->transform->GlobalPosition().z > b->transform->GlobalPosition().z;
    // });
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
    for (int i = (int)_imageFutures.size() - 1; i >= 0; i--)
    {
        auto & [spriteIndex, future] = _imageFutures[i];
        auto result = future.wait_for(std::chrono::milliseconds(0));
        if (result == std::future_status::ready)
        {
            auto sprite = levelContent->Sprites()[spriteIndex];
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

                tUs[i].x = sprite->uvs[i].x;
                tUs[i].y = 1.0f - sprite->uvs[i].y;
            }

            auto gpuTexture = future.get();

            auto [imageData, tempData] = _spriteRenderer->Allocate(
                recordState.commandBuffer,
                *gpuTexture,
                (int)sprite->vertices.size(),
                tVs.data(),
                tUs.data(),
                sprite->color,
                (int)sprite->indices.size(),
                sprite->indices.data()
            );

            std::shared_ptr mySprite = std::make_shared<Sprite>();
            mySprite->transform = sprite->transform_ptr;
            mySprite->spriteData = std::move(imageData);
            mySprite->gpuTexture = gpuTexture;

            bool inserted = false;
            for (int i = 0; i < _sprites.size(); i++)
            {
                if (_sprites[i]->transform->GlobalPosition().z < mySprite->transform->GlobalPosition().z)
                {
                    _sprites.insert(_sprites.begin() + i, mySprite);
                    inserted = true;
                    break;
                }
            }
            if (inserted == false)
            {
                _sprites.emplace_back(mySprite);
            }

            _temps.emplace_back(std::move(tempData));

            _imageFutures.erase(_imageFutures.begin() + i);
        }
    }
}

//======================================================================================================================

void GameScene::Render(MFA::RT::CommandRecordState &recordState)
{
    auto const worldWidth = (_right - _left);
    auto const worldHeight = (_bottom - _top);
    auto const xCenter = (worldWidth / 2) + _left;

    auto const * device = LogicalDevice::Instance;
    auto const windowWidth = device->GetWindowWidth();
    auto const windowHeight = device->GetWindowHeight();
    auto const ratio = (float)windowWidth / (float)windowHeight;

    auto const newWidth = ratio * worldHeight ;
    auto const left = xCenter - newWidth / 2.0f;
    auto const right = xCenter + newWidth / 2.0f;

    auto const projection = glm::ortho(left, right, _bottom, _top, _near, _far);
    auto const view = glm::lookAt(_mainCameraPosition, _mainCameraPosition + Math::ForwardVec3, -Math::UpVec3);
    auto const viewProjection = projection * view;

    SpritePipeline::PushConstants pushConstants {
        .model = glm::transpose(viewProjection)
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
void GameScene::ButtonA_Changed(bool const value)
{
    if (value == true)
    {
        _params.nextLevel();
    }
}
void GameScene::ButtonB_Pressed(bool const value)
{
    if (value == true)
    {
        _params.backPressed();
    }
}

//======================================================================================================================
