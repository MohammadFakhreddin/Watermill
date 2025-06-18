#include "GameScene.hpp"

#include "BedrockMath.hpp"
#include "BedrockPath.hpp"
#include "GenerateGame.h"
#include "LogicalDevice.hpp"
#include "camera/ObserverCamera.hpp"
#include "ResourceManager.hpp"
#include "JobSystem.hpp"

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
    MFA_LOG_INFO("Loading level %s", _params.levelName.c_str());

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
    _timeText = _webViewContainer->GetElementById("time");

    // TODO: We can sort images based on size and load the largest ones last as well
    auto levelPath = MFA::Path::Instance()->Get(_params.levelPath);
    MFA_ASSERT(std::filesystem::exists(levelPath) == true);

    _jsonTask = JobSystem::Instance()->AssignTask<LevelParser>([levelPath]()
    {
        return LevelParser(levelPath);
    });
}

//======================================================================================================================

void GameScene::Update(float const deltaTime)
{
    _webViewContainer->Update();

    _passedTime += deltaTime;

    {
        std::string text{};
        MFA_STRING(text, "Time: %d", (int)_passedTime);
        _webViewContainer->SetText(_timeText, text.c_str());
    }

    if (_jsonTask.valid() == true)
    {
        auto status = _jsonTask.wait_for(std::chrono::nanoseconds(0));
        if (status == std::future_status::ready)
        {
            levelContent = _jsonTask.get();
            ReadLevelFromJson();
        }
    }
}

//======================================================================================================================

void GameScene::UpdateBuffer(MFA::RT::CommandRecordState &recordState)
{
    _webViewContainer->UpdateBuffer(recordState);

    while (_nextUpdateTasks.IsEmpty() == false)
    {
        auto task = _nextUpdateTasks.Pop();
        task(recordState);
    }

    for (int i = _temporaryMemories.size() - 1; i >= 0; i--)
    {
        --_temporaryMemories[i].lifeTime;
        if (_temporaryMemories[i].lifeTime <= 0)
        {
            _temporaryMemories.erase(_temporaryMemories.begin() + i);
        }
    }
}

//======================================================================================================================

void GameScene::Render(MFA::RT::CommandRecordState &recordState)
{
    auto const worldWidth = (_cameraRight - _cameraLeft);
    auto const worldHeight = (_cameraTop - _cameraBottom);
    auto const xCenter = (worldWidth / 2) + _cameraLeft;
    // auto const yCenter = (worldHeight / 2) + _cameraBottom;

    auto const * device = LogicalDevice::Instance;
    auto const windowWidth = device->GetWindowWidth();
    auto const windowHeight = device->GetWindowHeight();
    auto const ratio = (float)windowWidth / (float)windowHeight;

    auto const newWidth = ratio * worldHeight ;
    auto const left = xCenter - newWidth / 2.0f;
    auto const right = xCenter + newWidth / 2.0f;
    // auto const newHeight = worldWidth / ratio;
    // auto const bottom = yCenter - newHeight * 0.5f;
    // auto const top = yCenter + newHeight * 0.5f;

    auto const projection = glm::ortho(left, right, _cameraBottom, _cameraTop, _cameraNear, _cameraFar);
    auto const view = glm::lookAt(_mainCameraPosition, _mainCameraPosition + Math::ForwardVec3, -Math::UpVec3);
    auto const viewProjection = projection * view;

    for (auto & instance : _instances)
    {
        SpritePipeline::PushConstants pushConstants {
            .color = instance->color,
            .model = glm::transpose(instance->transform->GlobalTransform() * instance->scaleMat),
            .viewProjection = glm::transpose(viewProjection),
        };
        _spriteRenderer->Draw(recordState, pushConstants, *instance->sprite->spriteData);
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

void GameScene::ReadLevelFromJson()
{
    MFA_LOG_INFO("ReadLevelFromJson start");

    _transforms = levelContent->GetTransforms();
    auto const & sprites = levelContent->GetSprites();

    for (int i = 0; i < (int)sprites.size(); ++i)
    {
        auto const address = Path::Instance()->Get("textures/" + sprites[i]->textureName);
        if (std::filesystem::exists(address))
        {
            int spriteIndex = i;
            ResourceManager::Instance()->RequestImage(address.c_str(), [spriteIndex, this](std::shared_ptr<RT::GpuTexture> gpuTexture)->void
            {
                JobSystem::Instance()->AssignTask([this, spriteIndex, gpuTexture]()->void
                {
                    auto sprite = levelContent->GetSprites()[spriteIndex];

                    std::vector<SpritePipeline::Position> tVs(sprite->vertices.size());
                    std::vector<SpriteRenderer::UV> tUs(sprite->vertices.size());
                    for (int i = 0; i < sprite->vertices.size(); ++i)
                    {
                        auto & oV = sprite->vertices[i];
                        auto & tV = tVs[i];

                        tV = glm::vec4{oV, 1.0f};

                        tUs[i].x = sprite->uvs[i].x;
                        tUs[i].y = 1.0f - sprite->uvs[i].y;
                    }

                    auto * logicalDevice = LogicalDevice::Instance;
                    auto * device = logicalDevice->GetVkDevice();
                    auto * commandPool = logicalDevice->GetGraphicCommandPool();

                    auto commandBufferGroup = RB::BeginSecondaryCommand(
                        device,
                        commandPool
                    );

                    auto commandBuffer = commandBufferGroup->commandBuffers[0];

                    auto [imageData_, tempData_] = _spriteRenderer->Allocate(
                        commandBuffer,
                        *gpuTexture,
                        (int)tVs.size(),
                        tVs.data(),
                        tUs.data(),
                        (int)sprite->indices.size(),
                        sprite->indices.data()
                    );

                    RB::EndCommandBuffer(commandBuffer);

                    std::shared_ptr imageData = std::move(imageData_);
                    std::shared_ptr tempData = std::move(tempData_);

                    _nextUpdateTasks.Push([this, spriteIndex, tempData, imageData, gpuTexture, commandBufferGroup](MFA::RenderTypes::CommandRecordState & recordState)->void
                    {
                        vkCmdExecuteCommands(
                            recordState.commandBuffer,
                            commandBufferGroup->commandBuffers.size(),
                            commandBufferGroup->commandBuffers.data()
                        );

                        _temporaryMemories.emplace_back(TemporaryMemory{
                            .lifeTime = (int)LogicalDevice::Instance->GetMaxFramePerFlight() + 1,
                            .memory = std::move(tempData),
                            .commandBuffer = std::move(commandBufferGroup)
                        });

                        std::shared_ptr mySprite = std::make_shared<Sprite>();
                        mySprite->spriteData = imageData;
                        mySprite->gpuTexture = gpuTexture;

                        _sprites.emplace_back(mySprite);

                        auto const & instances = levelContent->GetSpriteInstances(spriteIndex);
                        for (auto const & instance : instances)
                        {
                            glm::vec3 flipScale {1.0f, 1.0f, 1.0f};
                            if (instance->flipX == true)
                            {
                                flipScale.x *= -1.0f;
                            }
                            if (instance->flipY == true)
                            {
                                flipScale.y *= -1.0f;
                            }
                            auto scaleMat = glm::scale(glm::mat4(1), flipScale);

                            auto myInstance = std::make_shared<SpriteInstance>();
                            myInstance->scaleMat = scaleMat;
                            myInstance->sprite = mySprite.get();
                            myInstance->transform = instance->transform;
                            myInstance->color = instance->color;

                            bool inserted = false;
                            for (int i = 0 ; i < _instances.size(); i++)
                            {
                                if (_instances[i]->transform->GlobalPosition().z < myInstance->transform->GlobalPosition().z)
                                {
                                    _instances.insert(_instances.begin() + i, myInstance);
                                    inserted = true;
                                    break;
                                }
                            }
                            if (inserted == false)
                            {
                                _instances.emplace_back(std::move(myInstance));
                            }
                        }
                    });

                });
            });
        }
        else
        {
            MFA_LOG_WARN("Failed to find the asset with name: %s", address.c_str());
        }
    }

    auto const & cameras = levelContent->GetCameras();
    if (cameras.empty() == false)
    {
        if (cameras.size() > 1)
        {
            MFA_LOG_WARN("More than one camera detected");
        }

        auto & mainCamera = cameras[0];

        _cameraLeft = mainCamera->right;
        _cameraRight = mainCamera->left;
        auto xCenter = (_cameraLeft + _cameraRight) / 2.0f;
        _cameraLeft -= xCenter;
        _cameraRight -= xCenter;

        _cameraBottom = mainCamera->bottom;
        _cameraTop = mainCamera->top;
        auto yCenter = (_cameraBottom + _cameraTop) / 2.0f;
        _cameraBottom -= yCenter;
        _cameraTop -= yCenter;

        _cameraNear = mainCamera->near;
        _cameraFar = mainCamera->far;

        _mainCameraPosition = mainCamera->transform->GlobalPosition();
    }
}

//======================================================================================================================
