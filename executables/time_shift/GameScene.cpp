#include "GameScene.hpp"

#include "BedrockMath.hpp"
#include "BedrockPath.hpp"
#include "GenerateGame.h"
#include "LogicalDevice.hpp"
#include "camera/ObserverCamera.hpp"
#include "ResourceManager.hpp"
#include "JobSystem.hpp"
#include "Time.hpp"

#include <iostream>
#include <utility>

using namespace MFA;

//======================================================================================================================

GameScene::GameScene(WebViewContainer::Params const &webviewParams, Params gameParams) : _params(std::move(gameParams))
{
    MFA_LOG_INFO("Loading level %s", _params.levelName.c_str());

    auto const *device = MFA::LogicalDevice::Instance;

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

    _jsonTask = JobSystem::Instance()->AssignTask<LevelParser>([levelPath]() { return LevelParser(levelPath); });
}

GameScene::~GameScene()
{
    MFA_LOG_INFO("Destroying level %s", _params.levelName.c_str());
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

    std::weak_ptr sceneRef = shared_from_this();

    for (int i = 0; i < (int)sprites.size(); ++i)
    {
        auto const address = Path::Instance()->Get("textures/" + sprites[i]->textureName);
        if (std::filesystem::exists(address))
        {
            int spriteIndex = i;
            ResourceManager::Instance()->RequestImage(address.c_str(), [spriteIndex, sceneRef](std::shared_ptr<RT::GpuTexture> gpuTexture)->void
            {
                JobSystem::Instance()->AssignTask([sceneRef, spriteIndex, gpuTexture = std::move(gpuTexture)]()->void
                {
                    auto scenePtr = sceneRef.lock();
                    if (scenePtr == nullptr)
                    {
                        return;
                    }
                    auto * scene = (GameScene *)scenePtr.get();

                    auto sprite = scene->levelContent->GetSprites()[spriteIndex];

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

                    MFA_SCOPE_LOCK(commandPool->lock);

                    auto commandBufferGroup = RB::BeginSecondaryCommand(
                        device,
                        *commandPool
                    );

                    auto commandBuffer = commandBufferGroup->commandBuffers[0];

                    auto [imageData_, tempData_] = scene->_spriteRenderer->Allocate(
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

                    LogicalDevice::Instance->AddRenderTask([
                        sceneRef,
                        spriteIndex,
                        tempData,
                        imageData,
                        gpuTexture = std::move(gpuTexture),
                        commandBufferGroup
                    ](MFA::RT::CommandRecordState & recordState)->bool
                    {
                        auto scenePtr = sceneRef.lock();
                        if (scenePtr == nullptr)
                        {
                            return false;
                        }
                        auto * scene = (GameScene *)scenePtr.get();

                        vkCmdExecuteCommands(
                            recordState.commandBuffer,
                            commandBufferGroup->commandBuffers.size(),
                            commandBufferGroup->commandBuffers.data()
                        );

                        struct TemporaryMemory
                        {
                            int lifeTime = 0;
                            std::shared_ptr<SpriteRenderer::CommandBufferData> memory;
                            std::shared_ptr<MFA::RT::CommandBufferGroup> commandBuffer;
                        };
                        auto tempMemory = std::make_shared<TemporaryMemory>(TemporaryMemory{
                            .lifeTime = (int)LogicalDevice::Instance->GetMaxFramePerFlight() + 1,
                            .memory = tempData,
                            .commandBuffer = commandBufferGroup
                        });
                        Time::AddUpdateTask([tempMemory]()->bool
                        {
                            tempMemory->lifeTime -= 1;
                            if (tempMemory->lifeTime <= 0)
                            {
                                return false;
                            }
                            return true;
                        });

                        std::shared_ptr mySprite = std::make_shared<Sprite>();
                        mySprite->spriteData = imageData;
                        mySprite->gpuTexture = gpuTexture;

                        scene->_sprites.emplace_back(mySprite);

                        auto const & instances = scene->levelContent->GetSpriteInstances(spriteIndex);
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
                            for (int i = 0 ; i < scene->_instances.size(); i++)
                            {
                                if (scene->_instances[i]->transform->GlobalPosition().z < myInstance->transform->GlobalPosition().z)
                                {
                                    scene->_instances.insert(scene->_instances.begin() + i, myInstance);
                                    inserted = true;
                                    break;
                                }
                            }
                            if (inserted == false)
                            {
                                scene->_instances.emplace_back(std::move(myInstance));
                            }
                        }

                        return false;
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
