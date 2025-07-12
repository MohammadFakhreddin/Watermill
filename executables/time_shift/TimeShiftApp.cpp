#include "TimeShiftApp.hpp"

#include "BedrockFile.hpp"
#include "BedrockPath.hpp"
#include "GameScene.hpp"
#include "ImportTexture.hpp"
#include "LogicalDevice.hpp"
#include "MenuScene.hpp"
#include "ResourceManager.hpp"
#include "ScopeLock.hpp"
#include "ScoreboardScene.hpp"
#include "SpritePipeline.hpp"
#include "SpriteRenderer.hpp"
#include "Time.hpp"
#include "renderer/BorderPipeline.hpp"
#include "renderer/BorderRenderer.hpp"
#include "renderer/SolidFillPipeline.hpp"
#include "renderer/SolidFillRenderer.hpp"


using namespace MFA;

//=============================================================

void TimeShiftApp::Run()
{
    auto* device = LogicalDevice::Instance;

    device->SDL_EventSignal.Register([this](SDL_Event* event)->void{OnSDL_Event(event);});

    auto const swapChainResource = std::make_shared<SwapChainRenderResource>();
    auto const depthResource = std::make_shared<DepthRenderResource>();
    auto const msaaResource = std::make_shared<MSSAA_RenderResource>();
    _displayRenderPass = std::make_shared<DisplayRenderPass>(
        swapChainResource,
        depthResource,
        msaaResource
    );

    device->ResizeEventSignal2.Register([this]()->void {
        Resize();
    });

    InitFontPipeline();

    {// Solid fill
        auto const solidFillPipeline = std::make_shared<SolidFillPipeline>(_displayRenderPass);
        _solidFillRenderer = std::make_shared<SolidFillRenderer>(solidFillPipeline);
    }

    {// Border
        auto const borderPipeline = std::make_shared<BorderPipeline>(_displayRenderPass);
        _borderRenderer = std::make_shared<BorderRenderer>(borderPipeline);
    }

    auto const imageSampler = RB::CreateSampler(device->GetVkDevice(), {
        .minFilter = VK_FILTER_NEAREST,
        .magFilter = VK_FILTER_NEAREST,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .anisotropyEnabled = true,
        .maxAnisotropy = device->GetPhysicalDeviceProperties().limits.maxSamplerAnisotropy,
    });
    {// Image
        auto const imagePipeline = std::make_shared<ImagePipeline>(_displayRenderPass, imageSampler);
        _imageRenderer = std::make_shared<ImageRenderer>(imagePipeline);
    }
    {
        auto const spritePipeline = std::make_shared<SpritePipeline>(_displayRenderPass, imageSampler);
        _spriteRenderer = std::make_shared<SpriteRenderer>(spritePipeline);
    }

    {// Game scenes
        litehtml::position clip;
        clip.x = 0;
        clip.y = 0;
        clip.width = device->GetWindowWidth();
        clip.height = device->GetWindowHeight();

        WebViewContainer::Params webviewParams
        {
            .solidFillRenderer = _solidFillRenderer,
            .imageRenderer = _imageRenderer,
            .borderRenderer = _borderRenderer,
            .requestBlob = [this](char const *address, bool force) { return RequestBlob(address, force); },
            .requestFont = [this](char const * font) { return RequestFont(font); },
            .requestImage = [this](char const * image) {return RequestImage(image);}
        };

        _sceneRecipes.resize((int)SceneID::Count);

        std::weak_ptr weakRef = shared_from_this();
        _sceneRecipes[(int)SceneID::Menu] = [weakRef, webviewParams]()->std::shared_ptr<IScene>
        {
            MenuScene::Params menuParams
            {
                .PlayPressed = [weakRef]()->void
                {
                    auto myRef = weakRef.lock();
                    if (myRef != nullptr)
                    {
                        MFA_LOG_INFO("Play pressed");
                        myRef->_nextSceneID = SceneID::Level1;
                    }
                },
                .ScoreBoardPressed = [weakRef]()->void
                {
                    auto myRef = weakRef.lock();
                    if (myRef != nullptr)
                    {
                        MFA_LOG_INFO("Scoreboard pressed");
                        myRef->_nextSceneID = SceneID::Scoreboard;
                    }
                },
            };

            return std::make_shared<MenuScene>(webviewParams, menuParams);
        };

        _sceneRecipes[(int)SceneID::Scoreboard] = [weakRef, webviewParams]()->std::shared_ptr<IScene>
        {

            ScoreboardScene::Params scoreboardParams
            {
                .BackPressed = [weakRef]()->void
                {
                    auto myRef = weakRef.lock();
                    if (myRef != nullptr)
                    {
                        MFA_LOG_INFO("Back pressed");
                        myRef->_nextSceneID = SceneID::Menu;
                    }
                }
            };

            return std::make_shared<ScoreboardScene>(webviewParams, scoreboardParams);
        };

        _sceneRecipes[(int)SceneID::Level1] = [weakRef, webviewParams]()->std::shared_ptr<IScene>
        {
            auto myRef = weakRef.lock();
            if (myRef != nullptr)
            {
                GameScene::Params gameParams
                {
                    .levelName = "Level 1",
                    .levelPath = "levels/Level1.json",
                    .backPressed = [weakRef]()->void
                    {
                        auto myRef = weakRef.lock();
                        if (myRef != nullptr)
                        {
                            MFA_LOG_INFO("Back pressed");
                            myRef->_nextSceneID = SceneID::Menu;
                        }
                    },
                    .nextLevel = [weakRef]()->void
                    {
                        auto myRef = weakRef.lock();
                        if (myRef != nullptr)
                        {
                            myRef->_nextSceneID = SceneID::Level2;
                        }
                    },
                    .spriteRenderer = myRef->_spriteRenderer
                };

                return std::make_shared<GameScene>(webviewParams, gameParams);
            }
            return nullptr;
        };

        _sceneRecipes[(int)SceneID::Level2] = [weakRef, webviewParams]()->std::shared_ptr<IScene>
        {
            auto myRef = weakRef.lock();
            if (myRef != nullptr)
            {
                GameScene::Params gameParams
                {
                    .levelName = "Level 2",
                    .levelPath = "levels/Level2.json",
                    .backPressed = [weakRef]()->void
                    {
                        MFA_LOG_INFO("Back pressed");
                        auto myRef = weakRef.lock();
                        if (myRef != nullptr)
                        {
                            myRef->_nextSceneID = SceneID::Menu;
                        }
                    },
                    .nextLevel = [weakRef]()->void
                    {
                        auto myRef = weakRef.lock();
                        if (myRef != nullptr)
                        {
                            myRef->_nextSceneID = SceneID::Level3;
                        }
                    },
                    .spriteRenderer = myRef->_spriteRenderer
                };

                return std::make_shared<GameScene>(webviewParams, gameParams);
            }
            return nullptr;
        };

        _sceneRecipes[(int)SceneID::Level3] = [weakRef, webviewParams]()->std::shared_ptr<IScene>
        {
            auto myRef = weakRef.lock();
            if (myRef != nullptr)
            {
                GameScene::Params gameParams
                {
                    .levelName = "Level 3",
                    .levelPath = "levels/level3.json",
                    .backPressed = [weakRef]()->void
                    {
                        auto myRef = weakRef.lock();
                        if (myRef != nullptr)
                        {
                            MFA_LOG_INFO("Back pressed");
                            myRef->_nextSceneID = SceneID::Menu;
                        }
                    },
                    .nextLevel = [weakRef]()->void
                    {
                        auto myRef = weakRef.lock();
                        if (myRef != nullptr)
                        {
                            myRef->_nextSceneID = SceneID::Scoreboard;
                        }
                    },
                    .spriteRenderer = myRef->_spriteRenderer
                };

                return std::make_shared<GameScene>(webviewParams, gameParams);
            }
            return nullptr;
        };

        _nextSceneID = SceneID::Menu;
    }

    MFA_LOG_INFO("Initializing game loop");

    SDL_GL_SetSwapInterval(0);
    SDL_Event e;

    auto time = Time::Instantiate(120, 30);

    bool shouldQuit = false;

    while (shouldQuit == false)
    {
        //Handle events
        while (SDL_PollEvent(&e) != 0)
        {
            //User requests quit
            if (e.type == SDL_QUIT)
            {
                shouldQuit = true;
            }
        }

        device->Update();

    	Update(Time::DeltaTimeSec());

        auto recordState = device->AcquireRecordState(swapChainResource->GetSwapChainImages().swapChain);
        if (recordState.isValid == true)
        {
            Render(recordState);
            device->SubmitQueues(recordState);
            device->Present(recordState, swapChainResource->GetSwapChainImages().swapChain);
        }

        time->Update();
    }

    time.reset();

    device->DeviceWaitIdle();
}

//=============================================================

void TimeShiftApp::Update(float const deltaTime)
{
    if (_activeSceneID != _nextSceneID)
    {
        MFA_LOG_INFO("Switching to next scene");
        // ResourceManager::Instance()->ForceCleanUp();
        _blobMap.clear();
        _imageMap.clear();
        // TODO: We have to load stuff in another thread
        _previousScenes.emplace_back(std::tuple{std::move(_currentScene), LogicalDevice::Instance->GetMaxFramePerFlight() + 1});
        _currentScene = _sceneRecipes[(int)_nextSceneID]();
        _activeSceneID = _nextSceneID;
    }

    _currentScene->Update(deltaTime);

    for (int i = (int)_previousScenes.size() - 1; i >= 0; i--)
    {
        auto & [oldScene, counter] = _previousScenes[i];
        --counter;
        if (counter <= 0)
        {
            _previousScenes.erase(_previousScenes.begin() + i);
        }
    }

    //
    // MFA_LOG_INFO(
    //     "Input axis: %f, %f\nInput A: %d, Input B: %d"
    //     , _inputAxis.x
    //     , _inputAxis.y
    //     , _inputA == true ? 1 : 0
    //     , _inputB == true ? 1 : 0
    // );
}

//=============================================================

void TimeShiftApp::Render(RT::CommandRecordState & recordState)
{
    auto* device = LogicalDevice::Instance;

    device->BeginCommandBuffer(
        recordState,
        RT::CommandBufferType::Graphic
    );

    _currentScene->UpdateBuffer(recordState);
    _displayRenderPass->Begin(recordState);
    _currentScene->Render(recordState);
    _displayRenderPass->End(recordState);

    device->EndCommandBuffer(recordState);
}

//=============================================================

void TimeShiftApp::Resize()
{
    auto const * device = LogicalDevice::Instance;
    RB::DeviceWaitIdle(device->GetVkDevice());

    if (_currentScene != nullptr)
    {
        _currentScene->Resize();
    }
}

//=============================================================

void TimeShiftApp::OnSDL_Event(SDL_Event* event)
{
    if (event->type == SDL_KEYDOWN)
    {
        if (event->key.keysym.sym == SDLK_F5)
        {
            Reload();
        }
        else if (event->key.keysym.sym == SDLK_F1)
        {
            _nextSceneID = SceneID::Menu;
        }
    }

    bool inputAxisChanged = false;
    bool inputA_Changed = false;
    bool inputB_Changed = false;

    {// Keyboard
        if (event->type == SDL_KEYDOWN || event->type == SDL_KEYUP)
        {
            auto const modifier = event->type == SDL_KEYDOWN ? 1.0f : -1.0f;
            if (event->key.keysym.sym == SDLK_LEFT || event->key.keysym.sym == SDLK_RIGHT)
            {
                if (event->key.keysym.sym == SDLK_LEFT)
                {
                    _inputAxis.x -= modifier;
                }
                else if (event->key.keysym.sym == SDLK_RIGHT)
                {
                    _inputAxis.x += modifier;
                }
                _inputAxis.x = std::clamp(_inputAxis.x, -1.0f, 1.0f);
                inputAxisChanged = true;
            }
            else if (event->key.keysym.sym == SDLK_UP || event->key.keysym.sym == SDLK_DOWN)
            {
                if (event->key.keysym.sym == SDLK_UP)
                {
                    _inputAxis.y += modifier;
                }
                else if (event->key.keysym.sym == SDLK_DOWN)
                {
                    _inputAxis.y -= modifier;
                }
                _inputAxis.y = std::clamp(_inputAxis.y, -1.0f, 1.0f);
                inputAxisChanged = true;
            }
            else if(event->key.keysym.sym == SDLK_SPACE)
            {
                _inputA = modifier > 0;
                inputA_Changed = true;
            }
            else if (event->key.keysym.sym == SDLK_ESCAPE)
            {
                _inputB = modifier > 0;
                inputB_Changed = true;
            }
        }
    }

    auto ProcessJoystickAxis = [](Sint16 joyAxisValue)->float
    {
        constexpr Sint16 JOYSTICK_DEADZONE = 8000;
        return joyAxisValue < -JOYSTICK_DEADZONE ? -1.0 : joyAxisValue > JOYSTICK_DEADZONE ? 1.0 : 0.0;
    };

    {// Joystick
        if (event->type == SDL_JOYAXISMOTION)
        {
            if (event->jaxis.axis == 0)
            {
                _inputAxis.x = ProcessJoystickAxis(event->jaxis.value);
                inputAxisChanged = true;
            }
            else if (event->jaxis.axis == 1)
            {
                _inputAxis.y = ProcessJoystickAxis(event->jaxis.value);
                inputAxisChanged = true;
            }
        }

        if (event->type == SDL_JOYBUTTONDOWN || event->type == SDL_JOYBUTTONUP)
        {
            auto const modifier = (event->type == SDL_JOYBUTTONDOWN) ? 1.0f : -1.0f;
            if (event->jbutton.button == 0 /* BUTTON A */)
            {
                _inputA = modifier > 0.0f;
                inputA_Changed = true;
            }
            else if (event->jbutton.button == 1 /* BUTTON B */)
            {
                _inputB = modifier > 0.0f;
                inputB_Changed = true;
            }
        }
    }

    if (inputAxisChanged == true)
    {
        _currentScene->UpdateInputAxis(_inputAxis);
    }

    if (inputA_Changed == true)
    {
        _currentScene->ButtonA_Changed(_inputA);
    }

    if (inputB_Changed == true)
    {
        _currentScene->ButtonB_Pressed(_inputB);
    }
}

//=============================================================

void TimeShiftApp::Reload()
{
    auto const * device = LogicalDevice::Instance;
    RB::DeviceWaitIdle(device->GetVkDevice());

    // TODO: Reload shaders too

    // for (auto * scene : _scenes)
    // {
    //     scene->Reload();
    // }
    _currentScene->Reload();
}

//=============================================================

void TimeShiftApp::InitFontPipeline()
{
    auto *device = LogicalDevice::Instance;
    // Font
    RB::CreateSamplerParams fontSamplerParams{};
    fontSamplerParams.magFilter = VK_FILTER_NEAREST;
    fontSamplerParams.minFilter = VK_FILTER_NEAREST;
    fontSamplerParams.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    fontSamplerParams.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    fontSamplerParams.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    fontSamplerParams.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    fontSamplerParams.mipLodBias = 0.0f;
    fontSamplerParams.compareOp = VK_COMPARE_OP_NEVER;
    fontSamplerParams.minLod = 0.0f;
    fontSamplerParams.maxLod = 1.0f;
    fontSamplerParams.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    fontSamplerParams.maxAnisotropy = device->GetPhysicalDeviceProperties().limits.maxSamplerAnisotropy;

    auto const fontSampler = RB::CreateSampler(device->GetVkDevice(), fontSamplerParams);
    MFA_ASSERT(fontSampler != nullptr);
    _fontPipeline = std::make_shared<TextOverlayPipeline>(_displayRenderPass, fontSampler);
    AddFont("JetBrainsMono", Path::Instance()->Get("fonts/JetBrains-Mono/JetBrainsMono-Bold.ttf").c_str());
    AddFont("PublicSans", Path::Instance()->Get("fonts/PublicSans/PublicSans-Bold.ttf").c_str());
}

//=============================================================

void TimeShiftApp::AddFont(char const *name, char const *path)
{
    MFA_ASSERT(_fontMap.contains(name) == false);
    MFA_ASSERT(std::filesystem::exists(path) == true);
    auto const fontData = File::Read(path);
    _fontMap[name] = std::make_shared<CustomFontRenderer>(
        _fontPipeline,
        Alias{fontData->Ptr(), fontData->Len()},
        200.0f
    );
}

// TODO: We have to reuse stuff here instead
//=============================================================

std::shared_ptr<Blob> TimeShiftApp::RequestBlob(char const *address, bool const ignoreCache)
{
    if (ignoreCache == false)
    {
        auto const findResult = _blobMap.find(address);
        if (findResult != _blobMap.end())
        {
            return findResult->second;
        }
    }
    auto const blob = File::Read(address);
    _blobMap[address] = blob;
    return blob;
}

//=============================================================

std::shared_ptr<CustomFontRenderer> TimeShiftApp::RequestFont(char const *font)
{
    auto const findResult = _fontMap.find(font);
    if (findResult != _fontMap.end())
    {
        return findResult->second;
    }
    // MFA_LOG_WARN("Failed to find font with name %s", font);
    return _fontMap.begin()->second;
}

//=============================================================

std::tuple<std::shared_ptr<RT::GpuTexture>, glm::vec2> TimeShiftApp::RequestImage(char const *imageName)
{
    // TODO: This should call the resource manager
    auto const findResult = _imageMap.find(imageName);
    if (findResult != _imageMap.end())
    {
        return findResult->second;
    }
    // In this case we want to render a 3d scene as well.
    if (strncmp(imageName, "scene", strlen("scene")) == 0)
    {
        MFA_LOG_ERROR("Not implemented yet!");
    }
    else
    {
        auto * device = LogicalDevice::Instance;

        auto const path = Path::Instance()->Get(imageName);

        auto const commandBuffer = RB::BeginSingleTimeCommand(device->GetVkDevice(), *device->GetGraphicCommandPool());

        auto const cpuTexture = Importer::UncompressedImage(path);

        std::vector<uint8_t> mipLevels{0};
        auto [gpuTexture, stageBuffer] = RB::CreateTexture(
            *cpuTexture,
            device->GetVkDevice(),
            device->GetPhysicalDevice(),
            commandBuffer,
            mipLevels.size(),
            mipLevels.data()
        );

        RB::EndAndSubmitSingleTimeCommand(
            device->GetVkDevice(),
            *device->GetGraphicCommandPool(),
            device->GetGraphicQueue(),
            commandBuffer
        );

        auto const & mipDim = cpuTexture->GetMipmapDimension(mipLevels[0]);

        auto const tuple = std::tuple{gpuTexture, glm::vec2{mipDim.width, mipDim.height}};

        _imageMap[imageName] = tuple;

        return tuple;
    }

    return {};
}

//=============================================================
