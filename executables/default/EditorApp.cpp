#include "EditorApp.hpp"

#include "Buffers.hpp"

using namespace MFA;

//======================================================================================================================

EditorApp::EditorApp()
{
    _path = Path::Instance();
    _device = LogicalDevice::Instance;

    if (SDL_JoystickOpen(0) != nullptr)
        SDL_JoystickEventState(SDL_ENABLE);

    _device->SDL_EventSignal.Register([&](SDL_Event *event) -> void { OnSDL_Event(event); });

    _swapChainResource = std::make_shared<SwapChainRenderResource>();
    _depthResource = std::make_shared<DepthRenderResource>();
    _msaaResource = std::make_shared<MSSAA_RenderResource>();
    _displayRenderPass = std::make_shared<DisplayRenderPass>(_swapChainResource, _depthResource, _msaaResource);

    _sampler = RB::CreateSampler(_device->GetVkDevice(), RB::CreateSamplerParams{});

    _ui = std::make_shared<UI>(_displayRenderPass, UI::Params {
        .lightMode = false,
        .fontCallback = [this](ImGuiIO & io)->void
        {
            {// Default font
                auto const fontPath = Path::Instance()->Get("fonts/JetBrains-Mono/JetBrainsMonoNL-Regular.ttf");
                MFA_ASSERT(std::filesystem::exists(fontPath));
                _defaultFont = io.Fonts->AddFontFromFileTTF(
                    fontPath.c_str(),
                    20.0f
                );
                MFA_ASSERT(_defaultFont != nullptr);
            }
            {// Bold font
                auto const fontPath = Path::Instance()->Get("fonts/JetBrains-Mono/JetBrainsMono-Bold.ttf");
                MFA_ASSERT(std::filesystem::exists(fontPath));
                _boldFont = io.Fonts->AddFontFromFileTTF(
                    fontPath.c_str(),
                    20.0f
                );
                MFA_ASSERT(_boldFont != nullptr);
            }
        }
    });
    _ui->UpdateSignal.Register([this]() -> void { OnUI(Time::DeltaTimeSec()); });

    _device->ResizeEventSignal2.Register([this]() -> void { Resize(); });

    PrepareEditorRenderPass();
    PrepareGameRenderPass();

    {// Editor
        auto cameraBufferGroup = RB::CreateHostVisibleUniformBuffer(
            _device->GetVkDevice(),
            _device->GetPhysicalDevice(),
            sizeof(Buffers::Camera),
            _device->GetMaxFramePerFlight()
        );
        _editorCameraBufferTracker = std::make_shared<HostVisibleBufferTracker>(cameraBufferGroup);

        _editorCamera = std::make_unique<MFA::ArcballCamera>(
            [this]()->VkExtent2D
            {
                return _editorWindowSize;
            },
            [this]()->bool{return _editorWindowFocused;},
            glm::vec3{},
            -Math::ForwardVec3
        );
        _editorCamera->SetfovDeg(40.0f);
        _editorCamera->SetLocalPosition(glm::vec3{20.0f, 20.0f, 20.0f});
        _editorCamera->SetfarPlane(1000.0f);
        _editorCamera->SetnearPlane(0.010f);
        _editorCamera->SetmaxDistance(100.0f);

        _editorGridPipeline = std::make_shared<GridPipeline>(_editorRenderPass->GetRenderPass());
        _editorGridRenderer = std::make_unique<GridRenderer>(_editorGridPipeline);
    }

    {// Game
        auto cameraBufferGroup = RB::CreateHostVisibleUniformBuffer(
            _device->GetVkDevice(),
            _device->GetPhysicalDevice(),
            sizeof(Buffers::Camera),
            _device->GetMaxFramePerFlight()
        );
        _gameCameraBufferTracker = std::make_shared<HostVisibleBufferTracker>(cameraBufferGroup);

        _gameCamera = std::make_unique<MFA::ArcballCamera>(
            [this]()->VkExtent2D
            {
                return _gameWindowSize;
            },
            [this]()->bool{return _gameWindowFocused;},
            glm::vec3{},
            -Math::ForwardVec3
        );
        _gameCamera->SetfovDeg(40.0f);
        _gameCamera->SetLocalPosition(glm::vec3{20.0f, 20.0f, 20.0f});
        _gameCamera->SetfarPlane(1000.0f);
        _gameCamera->SetnearPlane(0.010f);
        _gameCamera->SetmaxDistance(100.0f);
    }
}

//======================================================================================================================

EditorApp::~EditorApp() = default;

//======================================================================================================================

void EditorApp::Run()
{
    SDL_GL_SetSwapInterval(0);
    SDL_Event e;

    _time = Time::Instantiate(120, 30);

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

        _device->Update();

        Update(Time::DeltaTimeSec());

        auto recordState = _device->AcquireRecordState(_swapChainResource->GetSwapChainImages().swapChain);
        if (recordState.isValid == true)
        {
            _activeImageIndex = static_cast<int>(recordState.imageIndex);
            Render(recordState);
        }

        _time->Update();
    }

    _time.reset();

    _device->DeviceWaitIdle();
}

//======================================================================================================================

void EditorApp::Update(float deltaTime) {}

//======================================================================================================================

void EditorApp::Render(MFA::RT::CommandRecordState &recordState) {}

//======================================================================================================================

void EditorApp::Resize() {}

//======================================================================================================================

void EditorApp::Reload() {}

//======================================================================================================================

void EditorApp::OnSDL_Event(SDL_Event *event) {}

//======================================================================================================================

void EditorApp::OnUI(float deltaTimeSec) {}

//======================================================================================================================

void EditorApp::PrepareEditorRenderPass()
{
    PrepareRenderPass(
        _editorRenderResource,
        _editorRenderPass,
        _editorFrameBuffer,
        _editorTextureID_List,
        _editorWindowSize
    );
}

//======================================================================================================================

void EditorApp::PrepareGameRenderPass()
{
    PrepareRenderPass(
        _gameRenderResource,
        _gameRenderPass,
        _gameFrameBuffer,
        _gameTextureID_List,
        _gameWindowSize
    );
}

//======================================================================================================================

void EditorApp::PrepareRenderPass(
    std::shared_ptr<SceneRenderResource> & renderResource,
    std::shared_ptr<SceneRenderPass> & renderPass,
    std::shared_ptr<SceneFrameBuffer> & frameBuffer,
    std::vector<ImTextureID> & textureIDs,
    VkExtent2D const & windowSize
)
{
    auto const * device = LogicalDevice::Instance;

    auto const maxImageCount = device->GetSwapChainImageCount();

    if (renderResource != nullptr || renderPass != nullptr || frameBuffer != nullptr)
    {
        MFA_ASSERT(renderResource != nullptr);
        MFA_ASSERT(renderPass != nullptr);
        MFA_ASSERT(frameBuffer != nullptr);

        oldScenes.emplace_back();
        OldScene & oldScene = oldScenes.back();
        oldScene.renderResource = renderResource;
        oldScene.frameBuffer = frameBuffer;
        oldScene.textureIDs = textureIDs;
        oldScene.remLifeTime = static_cast<int>(maxImageCount) + 1;
        oldScenes.emplace_back(oldScene);
        textureIDs.clear();
    }

    auto const surfaceFormat = device->GetSurfaceFormat().format;
    auto const depthFormat = device->GetDepthFormat();
    auto const sampleCount = device->GetMaxSampleCount();

    renderResource = std::make_shared<SceneRenderResource>(
        windowSize,
        surfaceFormat,
        depthFormat,
        sampleCount
    );
    if (renderPass == nullptr)
    {
        renderPass = std::make_unique<SceneRenderPass>(surfaceFormat, depthFormat, sampleCount);
    }
    frameBuffer = std::make_shared<SceneFrameBuffer>(renderResource, renderPass->GetRenderPass());
    textureIDs.resize(maxImageCount);

    for (int imageIndex = 0; imageIndex < maxImageCount; imageIndex++)
    {
        auto & sceneTextureID = textureIDs[imageIndex];
        sceneTextureID = _ui->AddTexture(_sampler->sampler, renderResource->ColorImage(imageIndex).imageView->imageView);
    }
}

//======================================================================================================================

void EditorApp::ApplyUI_Style() {}

//======================================================================================================================

void EditorApp::DisplayParametersWindow() {}

//======================================================================================================================

void EditorApp::DisplayHierarchyWindow() {}

//======================================================================================================================

void EditorApp::DisplayEditorWindow() {}

//======================================================================================================================

void EditorApp::DisplayGameWindow() {}

//======================================================================================================================
