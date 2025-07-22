#include "GameScene.hpp"

#include "BedrockMath.hpp"
#include "BedrockPath.hpp"
#include "LevelParser.hpp"
#include "LogicalDevice.hpp"
#include "camera/ObserverCamera.hpp"
#include "ResourceManager.hpp"
#include "JobSystem.hpp"
#include "Time.hpp"
#include "ScopeProfiler.hpp"

#include <utility>

using namespace MFA;

//======================================================================================================================

GameScene::GameScene(WebViewContainer::Params const &webviewParams, Params gameParams) : _params(std::move(gameParams))
{
    // MFA_LOG_INFO("Loading level %s", _params.levelName.c_str());

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
}

GameScene::~GameScene()
{
    // MFA_LOG_INFO("Destroying level %s", _params.levelName.c_str());
}

//======================================================================================================================

void GameScene::Update(float const deltaTime)
{

    if (_initialized == false)
    {
        _initialized = true;
        // TODO: We can sort images based on size and load the largest ones last as well
        auto levelPath = MFA::Path::Instance()->Get(_params.levelPath);
        MFA_ASSERT(std::filesystem::exists(levelPath) == true);

        std::weak_ptr weakRef = shared_from_this();
        JobSystem::AssignTask([weakRef, levelPath]()
        {
            // MFA_SCOPE_Profiler("Level initialization");
            auto levelContent = std::make_shared<LevelParser>(levelPath);

            auto ref = weakRef.lock();
            if (ref != nullptr)
            {
                ((GameScene *)ref.get())->ReadLevelFromJson(std::move(levelContent));
            }
            return false;
        });
    }

    _webViewContainer->Update();

    _passedTime += deltaTime;

    {
        std::string text{};
        MFA_STRING(text, "Time: %d", (int)_passedTime);
        _webViewContainer->SetText(_timeText, text.c_str());
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
    if (_isReadyToRender == true)
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
            _spriteRenderer->Draw(recordState, pushConstants, *instance->sprite);
        }
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

void GameScene::ReadLevelFromJson(std::shared_ptr<LevelParser> levelParser)
{
    // MFA_SCOPE_Profiler("Read level from json")
    // TODO: Split buffers from sprites. There are some common sprites among them
    // TODO: We can furthermore separate texture from sprites.
    _transforms = levelParser->GetTransforms();

    std::weak_ptr sceneRef = shared_from_this();

    auto * logicalDevice = LogicalDevice::Instance;
    auto * device = logicalDevice->GetVkDevice();
    auto * commandPool = logicalDevice->GetGraphicCommandPool();

    MFA_SCOPE_LOCK(commandPool->lock);
    auto commandBufferGroup = RB::BeginSecondaryCommand(
        device,
        *commandPool
    );
    auto commandBuffer = commandBufferGroup->commandBuffers[0];

    auto errorTexture = ResourceManager::ErrorTexture();

    std::unordered_map<int, std::shared_ptr<RT::BufferAndMemory>> vertexBuffers{};
    std::unordered_map<int, std::tuple<std::shared_ptr<RT::BufferAndMemory>, int>> indexBuffers{};

    auto const & rawBuffers = levelParser->GetBuffers();
    auto const & rawBlob = levelParser->GetBlob();
    std::vector<std::shared_ptr<RT::BufferGroup>> stageBuffers;

    for (int bIdx = 0; bIdx < rawBuffers.size(); ++bIdx)
    {
        auto const & rawBuffer = rawBuffers[bIdx];

        auto const & bufferType = rawBuffer->type;
        auto const & bufferOffset = rawBuffer->offset;
        auto const & bufferSize = rawBuffer->size;

        MFA_ASSERT(bufferOffset >= 0);
        MFA_ASSERT(bufferSize > 0);
        MFA_ASSERT(rawBlob->IsValid());
        MFA_ASSERT((rawBlob->Len() >= bufferOffset + bufferSize));

        if (bufferType == LevelParser::BufferType::VertexBuffer)
        {
            auto levelVertices = reinterpret_cast<LevelParser::Vertex *>(rawBlob->Ptr() + bufferOffset);
            size_t oVsCount = bufferSize / sizeof(LevelParser::Vertex);
            MFA_ASSERT(oVsCount > 0);

            std::vector<SpritePipeline::Position> positions(oVsCount);
            std::vector<SpritePipeline::UV> uvs(oVsCount);
            for (size_t i = 0; i < oVsCount; ++i)
            {
                auto const & levelVertex = levelVertices[i];
                auto & position = positions[i];
                auto & uv = uvs[i];

                position = glm::vec4{levelVertex.position, 0.0f, 1.0f};

                uv.x = levelVertex.uv.x;
                uv.y = 1.0f - levelVertex.uv.y;
            }

            auto [vertexBuffer, vertexStageBuffer] = _spriteRenderer->AllocateVertexBuffer(
                commandBuffer,
                (int)positions.size(),
                positions.data(),
                uvs.data()
            );

            vertexBuffers.emplace(bIdx, std::move(vertexBuffer));
            stageBuffers.emplace_back(std::move(vertexStageBuffer));
        }
        else if (bufferType == LevelParser::BufferType::IndexBuffer)
        {
            auto levelIndices = reinterpret_cast<uint16_t *>(rawBlob->Ptr() + bufferOffset);
            size_t indicesCount = bufferSize / sizeof(uint16_t);
            MFA_ASSERT(indicesCount > 0);

            std::vector<uint16_t> indices(indicesCount);
            for (size_t i = 0; i < indicesCount; ++i)
            {
                indices[i] = levelIndices[i];
            }

            auto [indexBuffer, indexStageBuffer] = _spriteRenderer->AllocateIndexBuffer(
                commandBuffer,
                (int)indices.size(),
                indices.data()
            );

            indexBuffers.emplace(bIdx, std::tuple{std::move(indexBuffer), indicesCount});
            stageBuffers.emplace_back(indexStageBuffer);
        }
        else
        {
            MFA_ASSERT(false);
        }
    }

    auto const & jsonSprites = levelParser->GetSprites();
    for (int i = 0; i < (int)jsonSprites.size(); ++i)
    {
        auto & jsonSprite = jsonSprites[i];

        MFA_ASSERT(jsonSprite->vertexBufferIndex >= 0);
        auto vertexBuffer = vertexBuffers[jsonSprite->vertexBufferIndex];
        MFA_ASSERT(vertexBuffer != nullptr);

        MFA_ASSERT(jsonSprite->indexBufferIndex >= 0);
        auto [indexBuffer, indexCount] = indexBuffers[jsonSprite->indexBufferIndex];
        MFA_ASSERT(indexBuffer != nullptr);

        auto material = _spriteRenderer->AllocateMaterial(errorTexture);

        std::shared_ptr sprite = _spriteRenderer->CreateSprite(vertexBuffer, indexCount, indexBuffer, material);
        _sprites.emplace_back(sprite);

        auto const & instances = levelParser->GetSpriteInstances(i);
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
            myInstance->sprite = sprite.get();
            myInstance->transform = instance->transform;
            myInstance->color = instance->color;

            bool inserted = false;
            for (int j = 0 ; j < _instances.size(); j++)
            {
                if (_instances[j]->transform->GlobalPosition().z < myInstance->transform->GlobalPosition().z)
                {
                    _instances.insert(_instances.begin() + j, myInstance);
                    inserted = true;
                    break;
                }
            }
            if (inserted == false)
            {
                _instances.emplace_back(std::move(myInstance));
            }
        }

        auto const address = Path::Instance()->Get("textures/" + jsonSprites[i]->textureName);
        if (std::filesystem::exists(address))
        {
            int spriteIndex = i;
            ResourceManager::RequestImage(address.c_str(), [spriteIndex, sceneRef](std::shared_ptr<RT::GpuTexture> gpuTexture)->void
            {
                std::shared_ptr<int> counter = std::make_shared<int>(LogicalDevice::Instance->GetMaxFramePerFlight());
                LogicalDevice::AddRenderTask([spriteIndex, sceneRef, gpuTexture, counter](RT::CommandRecordState & recordState)->bool
                {
                    auto scenePtr = sceneRef.lock();
                    if (scenePtr == nullptr)
                    {
                       return false;
                    }
                    auto * scene = (GameScene *)scenePtr.get();
                    auto & sprite = scene->_sprites[spriteIndex];
                    scene->_spriteRenderer->UpdateMaterial(recordState.frameIndex, *sprite->material, gpuTexture);
                    (*counter) -= 1;
                    if (*counter <= 0)
                    {
                        // Remove the task from the queue
                        return false;
                    }
                    return true;
                });
            });
        }
    }

    RB::EndCommandBuffer(commandBuffer);

    logicalDevice->AddRenderTask([commandBufferGroup = std::move(commandBufferGroup), stageBuffers](RT::CommandRecordState & recordState)->bool
    {
        RB::ExecuteCommandBuffer(
            recordState.commandBuffer,
            *commandBufferGroup
        );

        std::shared_ptr<int> counter = std::make_shared<int>(LogicalDevice::Instance->GetMaxFramePerFlight() + 1);
        LogicalDevice::AddRenderTask([stageBuffers, commandBufferGroup, counter](RT::CommandRecordState const & recordState)->bool
        {
            (*counter) -= 1;
            if (*counter <= 0)
            {
                return false;
            }
            return true;
        });

        return false;
    });

    auto const & cameras = levelParser->GetCameras();
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

    _isReadyToRender = true;
}

//======================================================================================================================
