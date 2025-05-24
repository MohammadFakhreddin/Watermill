#include "TimeShiftApp.hpp"

#include "BedrockFile.hpp"
#include "BedrockPath.hpp"
#include "ImportTexture.hpp"
#include "LogicalDevice.hpp"
#include "Time.hpp"
#include "litehtml/el_text.h"

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

    {// Image
        auto const imageSampler = RB::CreateSampler(device->GetVkDevice(), {});
        auto const imagePipeline = std::make_shared<ImagePipeline>(_displayRenderPass, imageSampler);
        _imageRenderer = std::make_shared<ImageRenderer>(imagePipeline);
    }

    {// Game scenes
        litehtml::position clip;
        clip.x = 0;
        clip.y = 0;
        clip.width = device->GetWindowWidth();
        clip.height = device->GetWindowHeight();

        WebViewContainer::Params params
        {
            .solidFillRenderer = _solidFillRenderer,
            .imageRenderer = _imageRenderer,
            .borderRenderer = _borderRenderer,
            .requestBlob = [this](char const *address, bool force) { return RequestBlob(address, force); },
            .requestFont = [this](char const * font) { return RequestFont(font); },
            .requestImage = [this](char const * image) {return RequestImage(image);}
        };

        _menuScene = std::make_unique<MenuScene>(params);
        // _gameScene = std::make_unique<GameScene>(params);
        _scenes.emplace_back(_menuScene.get());
        _activeSceneIndex = 0;
    }

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
    _scenes[_activeSceneIndex]->Update(deltaTime);
}

//=============================================================

void TimeShiftApp::Render(RT::CommandRecordState & recordState)
{
    auto* device = LogicalDevice::Instance;
    // device->BeginCommandBuffer(
    //     recordState,
    //     RT::CommandBufferType::Compute
    // );
    // device->EndCommandBuffer(recordState);

    device->BeginCommandBuffer(
        recordState,
        RT::CommandBufferType::Graphic
    );

    _scenes[_activeSceneIndex]->UpdateBuffer(recordState);
    _displayRenderPass->Begin(recordState);
    _scenes[_activeSceneIndex]->Render(recordState);
    _displayRenderPass->End(recordState);

    device->EndCommandBuffer(recordState);
}

//=============================================================

void TimeShiftApp::Resize()
{
    auto const * device = LogicalDevice::Instance;
    RB::DeviceWaitIdle(device->GetVkDevice());

    for (auto * scene : _scenes)
    {
        scene->Resize();
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
        else if (event->key.keysym.sym == SDLK_DOWN)
        {
            // TODO
        }
        else if (event->key.keysym.sym == SDLK_UP)
        {
            // TODO
        }
        else if (event->key.keysym.sym == SDLK_RIGHT)
        {
            // TODO
        }
        else if (event->key.keysym.sym == SDLK_LEFT)
        {
            // TODO
        }
        else if (event->key.keysym.sym == SDLK_SPACE)
        {
            // TODO
        }
    }
}

//=============================================================

void TimeShiftApp::Reload()
{
    auto const * device = LogicalDevice::Instance;
    RB::DeviceWaitIdle(device->GetVkDevice());

    // TODO: Reload shaders too

    for (auto * scene : _scenes)
    {
        scene->Reload();
    }
}

//=============================================================

void TimeShiftApp::InitFontPipeline()
{
    auto *device = LogicalDevice::Instance;
    // Font
    RB::CreateSamplerParams fontSamplerParams{};
    fontSamplerParams.magFilter = VK_FILTER_LINEAR;
    fontSamplerParams.minFilter = VK_FILTER_LINEAR;
    fontSamplerParams.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    fontSamplerParams.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    fontSamplerParams.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    fontSamplerParams.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    fontSamplerParams.mipLodBias = 0.0f;
    fontSamplerParams.compareOp = VK_COMPARE_OP_NEVER;
    fontSamplerParams.minLod = 0.0f;
    fontSamplerParams.maxLod = 1.0f;
    fontSamplerParams.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;

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
    MFA_LOG_WARN("Failed to find font with name %s", font);
    return _fontMap.begin()->second;
}

//=============================================================

std::tuple<std::shared_ptr<RT::GpuTexture>, glm::vec2> TimeShiftApp::RequestImage(char const *imageName)
{
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

        auto const commandBuffer = RB::BeginSingleTimeCommand(device->GetVkDevice(), device->GetGraphicCommandPool());

        auto const cpuTexture = Importer::UncompressedImage(path);

        auto [gpuTexture, stageBuffer] = RB::CreateTexture(
            *cpuTexture,
            device->GetVkDevice(),
            device->GetPhysicalDevice(),
            commandBuffer
        );

        RB::EndAndSubmitSingleTimeCommand(
            device->GetVkDevice(),
            device->GetGraphicCommandPool(),
            device->GetGraphicQueue(),
            commandBuffer
        );

        auto const & mipDim = cpuTexture->GetMipmap(0).dimension;

        auto const tuple = std::tuple{gpuTexture, glm::vec2{mipDim.width, mipDim.height}};

        _imageMap[imageName] = tuple;

        return tuple;
    }

    return {};
}

//=============================================================
