#include "LogicalDevice.hpp"

#include "BedrockAssert.hpp"
#include "BedrockLog.hpp"
#include "BedrockPlatforms.hpp"
#include "RenderBackend.hpp"

#include <thread>

#define USE_VALIDATION_LAYERS

namespace MFA
{

    static VkBool32 VKAPI_PTR DebugCallback(
        VkDebugReportFlagsEXT const flags,
        VkDebugReportObjectTypeEXT object_type,
        uint64_t src_object,
        size_t location,
        int32_t const message_code,
        char const * player_prefix,
        char const * message,
        void * user_data
    )
    {
        if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
        {
            MFA_LOG_ERROR("Message code: %d\nMessage: %s\nLocation: %d\n", message_code, message, static_cast<int>(location));
            MFA_ASSERT(false);
            return true;
        }
        else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
        {
            MFA_LOG_WARN("Message code: %d\nMessage: %s\nLocation: %d\n", message_code, message, static_cast<int>(location));
        }
        else
        {
            MFA_LOG_INFO("Message code: %d\nMessage: %s\nLocation: %d\n", message_code, message, static_cast<int>(location));
        }
        return false;
    }

    //-------------------------------------------------------------------------------------------------

    static bool IsResizeEvent(uint8_t const sdlEvent)
    {
        return sdlEvent == SDL_WINDOWEVENT_RESIZED;
    }

    //-------------------------------------------------------------------------------------------------

    void LogicalDevice::OnResizeEvent(SDL_Event * event)
    {
        if (
            _resizable == true &&
            event->type == SDL_WINDOWEVENT &&
            IsResizeEvent(event->window.event)
        )
        {
            SDL_Window * sdlWindow = SDL_GetWindowFromID(event->window.windowID);
            if (sdlWindow == static_cast<SDL_Window *>(_window))
            {
                DeviceWaitIdle();
                _windowResized = true;
            }
        }
    }

    //-------------------------------------------------------------------------------------------------

    void LogicalDevice::Update()
    {
        auto const isWindowVisible = (SDL_GetWindowFlags(_window) & SDL_WINDOW_MINIMIZED) > 0 ? false : true;
        if (_windowVisible != isWindowVisible)
        {
            if (isWindowVisible)
            {
                UpdateSurface();
            }
            _windowVisible = isWindowVisible;
        }

        if (_windowResized)
        {
            UpdateSurface();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void LogicalDevice::UpdateSurface()
    {
        DeviceWaitIdle();

        _surfaceCapabilities = RB::GetSurfaceCapabilities(_physicalDevice, _surface);
        _windowWidth = static_cast<int>(_surfaceCapabilities.currentExtent.width);
        _windowHeight = static_cast<int>(_surfaceCapabilities.currentExtent.height);

        // Screen width and height can be equal to zero as well
        _windowResized = false;

         if (_windowWidth > 0 && _windowHeight > 0)
         {
             ResizeEventSignal1.Emit();
             ResizeEventSignal2.Emit();
         }
    }

    //-------------------------------------------------------------------------------------------------

    static int SDLEventWatcher(void * data, SDL_Event * event)
    {
        auto * device = LogicalDevice::Instance;
        if (device != nullptr)
        {
	        device->OnResizeEvent(event);
	        device->SDL_EventSignal.Emit(event);
        }
        return 0;
    }

    //-------------------------------------------------------------------------------------------------

    std::unique_ptr<LogicalDevice> LogicalDevice::Instantiate(InitParams const & params)
    {
        auto device = std::make_unique<LogicalDevice>(params);
        if (device->IsValid() == false)
        {
            return nullptr;
        }
        return device;
    }

    //-------------------------------------------------------------------------------------------------

    LogicalDevice::LogicalDevice(InitParams const & params)
    {
        _applicationName = params.applicationName;
        _windowWidth = params.windowWidth;
        _windowHeight = params.windowHeight;
        _fullScreen = params.fullScreen;
        _resizable = params.resizable;

        _window = RB::CreateWindow(
            _applicationName,
            _windowWidth,
            _windowHeight,
            0,
            0
        );
        MFA_ASSERT(_window != nullptr);

        if (_fullScreen)
        {
            SDL_SetWindowBordered(_window, SDL_FALSE);
        }

        if (_resizable)
        {
            // Make window resizable
            SDL_SetWindowResizable(_window, SDL_TRUE);
        }

        {// Creating window
            int displayWidth = 0;
            int displayHeight = 0;
            RB::GetScreenSize(displayWidth, displayHeight);

            if (_fullScreen == true)
            {
                _windowWidth = displayWidth;
                _windowHeight = displayHeight;
                SDL_SetWindowSize(_window, _windowWidth, _windowHeight);
            }
            else
            {
                auto windowPosX = displayWidth * 0.5f - _windowWidth * 0.5f;
                auto windowPosY = displayHeight * 0.5f - _windowHeight * 0.5f;
                SDL_SetWindowPosition(_window, windowPosX, windowPosY);
            }
        }

        SDL_SetWindowMinimumSize(_window, 100, 100);

        SDL_AddEventWatch(SDLEventWatcher, this);

        _vkInstance = RB::CreateInstance(
            _applicationName.c_str(),
            _window
        );

        MFA_ASSERT(_vkInstance != VK_NULL_HANDLE);

        _surface = RB::CreateWindowSurface(_window, _vkInstance);

        {// FindPhysicalDevice
            auto const findPhysicalDeviceResult = RB::FindBestPhysicalDevice(_vkInstance);   // TODO Check again for retry count number
            _physicalDevice = findPhysicalDeviceResult.physicalDevice;
            // I'm not sure if this is a correct thing to do but currently I'm enabling all gpu features.
            _physicalDeviceFeatures = findPhysicalDeviceResult.physicalDeviceFeatures;
            _maxSampleCount = findPhysicalDeviceResult.maxSampleCount;                    // TODO It should be a setting
            _physicalDeviceProperties = findPhysicalDeviceResult.physicalDeviceProperties;
            std::string message = "Supported physical device features are:";
            message += "\nSample rate shading support: ";
            message += _physicalDeviceFeatures.sampleRateShading ? "True" : "False";
            message += "\nSampler anisotropy support: ";
            message += _physicalDeviceFeatures.samplerAnisotropy ? "True" : "False";
            MFA_LOG_INFO("%s", message.c_str());
        }

        // Find surface capabilities
        _surfaceCapabilities = RB::GetSurfaceCapabilities(_physicalDevice, _surface);
        _swapChainImageCount = RB::ComputeSwapChainImagesCount(_surfaceCapabilities);
        // Temporary
        _maxFramePerFlight = _swapChainImageCount;//std::min(3u, _swapChainImageCount);

        MFA_LOG_INFO(
            "ScreenWidth: %d \nScreenHeight: %d",
            _surfaceCapabilities.currentExtent.width,
            _surfaceCapabilities.currentExtent.height
        );

        if (RB::CheckSwapChainSupport(_physicalDevice) == false)
        {
            MFA_LOG_ERROR("Swapchain is not supported on this device");
            return;
        }

        {// Trying to find queue family
            auto const result = RB::FindQueueFamilies(_physicalDevice, _surface);
            _graphicQueueFamily = result.graphicQueueFamily;
            _computeQueueFamily = result.computeQueueFamily;
            _presentQueueFamily = result.presentQueueFamily;
        }

        {
            auto result = RB::CreateLogicalDevice(
                _physicalDevice,
                _graphicQueueFamily,
                _presentQueueFamily,
                _physicalDeviceFeatures
            );
            _vkDevice = result.device;
            _physicalMemoryProperties = result.physicalMemoryProperties;
        }

        // Get graphics and presentation queues (which may be the same)
        _graphicQueue = RB::GetQueueByFamilyIndex(
            _vkDevice,
            _graphicQueueFamily
        );
        MFA_ASSERT(_graphicQueue != VK_NULL_HANDLE);

        _computeQueue = RB::GetQueueByFamilyIndex(
            _vkDevice,
            _computeQueueFamily
        );
        MFA_ASSERT(_computeQueue != VK_NULL_HANDLE);

        _presentQueue = RB::GetQueueByFamilyIndex(
            _vkDevice,
            _presentQueueFamily
        );
        MFA_ASSERT(_presentQueue != VK_NULL_HANDLE);

        MFA_LOG_INFO("Acquired graphics, compute and presentation queues");

        // Graphic
        _graphicCommandBuffer = RB::CreateCommandBuffers(
            _vkDevice,
            _maxFramePerFlight,
            *GetGraphicCommandPool()
        );

        _fences = RB::CreateFence(
            _vkDevice,
            _maxFramePerFlight
        );

        // Compute
        _computeCommandBuffer = RB::CreateCommandBuffers(
            _vkDevice,
            _maxFramePerFlight,
            *GetComputeCommandPool()
        );
        _computeSemaphores = RB::CreateSemaphores(
            _vkDevice,
            _maxFramePerFlight
        );

        // Presentation
        _presentSemaphores = RB::CreateSemaphores(
            _vkDevice,
            _maxFramePerFlight
        );

        _depthFormat = RB::FindDepthFormat(_physicalDevice);

    #if defined(MFA_DEBUG) and defined(USE_VALIDATION_LAYERS)
        _vkDebugReportCallbackExt = RB::CreateDebugCallback(
            _vkInstance,
            DebugCallback
        );
        MFA_LOG_INFO("Debug report callback are enabled");
    #endif

        _surfaceFormat = RB::ChooseSurfaceFormat(
			_physicalDevice,
            _surface,
            _surfaceCapabilities
        );

        _isValid = true;

        Instance = this;
    }

    //-------------------------------------------------------------------------------------------------

    LogicalDevice::~LogicalDevice()
    {

        // Common part with resize
        DeviceWaitIdle();

        {
            auto commandBuffer = RB::BeginSingleTimeCommand(_vkDevice, *GetGraphicCommandPool());

            RT::CommandRecordState recordState{
                .imageIndex = 0,
                .frameIndex = 0,
                .isValid = true,

                .commandBufferType = RenderTypes::CommandBufferType::Graphic,
                .commandBuffer = commandBuffer,
                .pipeline = nullptr,
                .renderPass = nullptr,
                .swapChain = VK_NULL_HANDLE,
            };

            {
                _renderTasks.PopAll();

                while (_pRenderTasks.empty() == false)
                {
                    auto task = _pRenderTasks.front();
                    _pRenderTasks.pop();
                }
            }

            RB::EndAndSubmitSingleTimeCommand(_vkDevice, *GetGraphicCommandPool(), GetGraphicQueue(), commandBuffer);
        }

        SDL_DelEventWatch(SDLEventWatcher, _window);

        // Graphic
        _graphicCommandBuffer.reset();
        _graphicCommandPoolMap.clear();
        // for (auto & [key, pool] : _graphicCommandPoolMap)
        // {
        //     RB::DestroyCommandPool(
        //         _vkDevice,
        //         pool
        //     );
        // }

        // Compute
        RB::DestroySemaphore(
            _vkDevice,
            _computeSemaphores
        );

        _computeCommandBuffer.reset();
        _computeCommandPoolMap.clear();
        // for (auto & [key, pool] : _computeCommandPoolMap)
        // {
        //     RB::DestroyCommandPool(
        //         _vkDevice,
        //         pool
        //     );
        // }

        // Presentation
        RB::DestroySemaphore(
            _vkDevice,
            _presentSemaphores
        );
        RB::DestroyFence(
            _vkDevice,
            _fences
        );

        RB::DestroyLogicalDevice(_vkDevice);

        RB::DestroyWindowSurface(_vkInstance, _surface);

    #if defined(MFA_DEBUG) and defined(USE_VALIDATION_LAYERS)
        RB::DestroyDebugReportCallback(_vkInstance, _vkDebugReportCallbackExt);
    #endif

        RB::DestroyInstance(_vkInstance);

        Instance = nullptr;
	}

    //-------------------------------------------------------------------------------------------------

    bool fenceIsSignaledOnce = false;
    RT::CommandRecordState LogicalDevice::AcquireRecordState(VkSwapchainKHR swapChain)
    {
	    MFA_ASSERT(_maxFramePerFlight > _currentFrame);
	    RT::CommandRecordState recordState{ .renderPass = nullptr };
	    if (_windowVisible == false || _windowResized == true)
	    {
		    recordState.isValid = false;
		    return recordState;
	    }

	    recordState.frameIndex = _currentFrame;
	    recordState.isValid = true;

        if (fenceIsSignaledOnce == false)
        {
            auto const fence = GetFence(recordState);
            RB::WaitForFence(_vkDevice, {fence});
            RB::ResetFences(_vkDevice, {fence});
        }

        // We ignore failed acquire of image because a resize will be triggered at end of pass
        auto const result = RB::AcquireNextImage(
            _vkDevice,
            GetPresentSemaphore(recordState),
            swapChain,
            recordState.imageIndex
        );
        fenceIsSignaledOnce = false;

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        {
            _windowResized = true;
            fenceIsSignaledOnce = true;
            recordState.isValid = false;
            return recordState;
        }

        ++_currentFrame;
        if (_currentFrame >= _maxFramePerFlight)
        {
            _currentFrame = 0;
        }

        recordState.swapChain = swapChain;

	    // Recording command buffer data at each render frame
	    // We need 1 renderPass and multiple command buffer recording
	    // Each pipeline has its own set of shader, But we can reuse a pipeline for multiple shaders.
	    // For each model we need to record command buffer with our desired pipeline (For example light and objects have different fragment shader)
	    // Prepare data for recording command buffers

	    return recordState;
    }

	//-------------------------------------------------------------------------------------------------

    bool LogicalDevice::IsValid() const noexcept
    {
	    return _isValid;
    }

    //-------------------------------------------------------------------------------------------------

    std::string LogicalDevice::ApplicationName() const noexcept
    {
	    return _applicationName;
    }

    //-------------------------------------------------------------------------------------------------

    bool LogicalDevice::IsResizable() const noexcept
    {
	    return _resizable;
    }

    //-------------------------------------------------------------------------------------------------

    bool LogicalDevice::IsWindowVisible() const noexcept
    {
	    return _windowVisible;
    }

    //-------------------------------------------------------------------------------------------------

    int LogicalDevice::GetWindowWidth() const noexcept
    {
	    return _windowWidth;
    }

    //-------------------------------------------------------------------------------------------------

    int LogicalDevice::GetWindowHeight() const noexcept
    {
	    return _windowHeight;
    }

    //-------------------------------------------------------------------------------------------------

    bool LogicalDevice::IsFullScreen() const noexcept
    {
	    return _fullScreen;
    }

    //-------------------------------------------------------------------------------------------------

    VkInstance LogicalDevice::GetVkInstance() const noexcept
    {
	    return _vkInstance;
    }

    //-------------------------------------------------------------------------------------------------

    VkSurfaceKHR LogicalDevice::GetSurface() const noexcept
    {
	    return _surface;
    }

    //-------------------------------------------------------------------------------------------------

    VkPhysicalDevice LogicalDevice::GetPhysicalDevice() const noexcept
    {
	    return _physicalDevice;
    }

    //-------------------------------------------------------------------------------------------------

    VkPhysicalDeviceFeatures LogicalDevice::GetPhysicalDeviceFeatures() const noexcept
    {
	    return _physicalDeviceFeatures;
    }

    //-------------------------------------------------------------------------------------------------

    VkSampleCountFlagBits LogicalDevice::GetMaxSampleCount() const noexcept
    {
	    return _maxSampleCount;
    }

    //-------------------------------------------------------------------------------------------------

    VkPhysicalDeviceProperties LogicalDevice::GetPhysicalDeviceProperties() const noexcept
    {
	    return _physicalDeviceProperties;
    }

    //-------------------------------------------------------------------------------------------------

    VkSurfaceCapabilitiesKHR LogicalDevice::GetSurfaceCapabilities() const noexcept
    {
	    return _surfaceCapabilities;
    }

    //-------------------------------------------------------------------------------------------------

    uint32_t LogicalDevice::GetSwapChainImageCount() const noexcept
    {
	    return _swapChainImageCount;
    }

    //-------------------------------------------------------------------------------------------------

    uint32_t LogicalDevice::GetMaxFramePerFlight() const noexcept
    {
	    return _maxFramePerFlight;
    }

    //-------------------------------------------------------------------------------------------------

    uint32_t LogicalDevice::GetGraphicQueueFamily() const noexcept
    {
	    return _graphicQueueFamily;
    }

    //-------------------------------------------------------------------------------------------------

    uint32_t LogicalDevice::GetComputeQueueFamily() const noexcept
    {
	    return _computeQueueFamily;
    }

    //-------------------------------------------------------------------------------------------------

    uint32_t LogicalDevice::GetPresentQueueFamily() const noexcept
    {
	    return _presentQueueFamily;
    }

    //-------------------------------------------------------------------------------------------------

    VkDevice LogicalDevice::GetVkDevice() const noexcept
    {
	    return _vkDevice;
    }

	//-------------------------------------------------------------------------------------------------

    VkPhysicalDeviceMemoryProperties LogicalDevice::GetPhysicalMemoryProperties() const noexcept
    {
	    return _physicalMemoryProperties;
    }

    //-------------------------------------------------------------------------------------------------

    VkQueue LogicalDevice::GetGraphicQueue() const noexcept
    {
	    return _graphicQueue;
    }

    //-------------------------------------------------------------------------------------------------

    VkQueue LogicalDevice::GetComputeQueue() const noexcept
    {
	    return _computeQueue;
    }

    //-------------------------------------------------------------------------------------------------

    VkQueue LogicalDevice::GetPresentQueue() const noexcept
    {
	    return _presentQueue;
    }

    //-------------------------------------------------------------------------------------------------

    RT::CommandPoolGroup * LogicalDevice::GetGraphicCommandPool()
    {
        RT::CommandPoolGroup * commandPoolRef;

        auto id = std::this_thread::get_id();
        auto findResult = _graphicCommandPoolMap.find(id);
        if (findResult == _graphicCommandPoolMap.end())
        {
            std::shared_ptr commandPool = RB::CreateCommandPool(_vkDevice, _graphicQueueFamily);
            _graphicCommandPoolMap[id] = commandPool;
            commandPoolRef = commandPool.get();
        }
        else
        {
            commandPoolRef = findResult->second.get();
        }

        return commandPoolRef;
    }

    //-------------------------------------------------------------------------------------------------

    std::vector<VkFence> const& LogicalDevice::GetGraphicFences() const noexcept
    {
	    return _fences;
    }

    //-------------------------------------------------------------------------------------------------

    RT::CommandPoolGroup * LogicalDevice::GetComputeCommandPool()
    {
        RT::CommandPoolGroup * commandPoolRef;

        auto id = std::this_thread::get_id();
        auto findResult = _computeCommandPoolMap.find(id);
        if (findResult == _computeCommandPoolMap.end())
        {
            std::shared_ptr commandPool = RB::CreateCommandPool(_vkDevice, _computeQueueFamily);
            _computeCommandPoolMap[id] = commandPool;
            commandPoolRef = commandPool.get();
        }
        else
        {
            commandPoolRef = findResult->second.get();
        }

        return commandPoolRef;
    }

    //-------------------------------------------------------------------------------------------------

    std::vector<VkSemaphore> const& LogicalDevice::GetComputeSemaphores() const noexcept
    {
	    return _computeSemaphores;
    }

    //-------------------------------------------------------------------------------------------------

    std::vector<VkSemaphore> const& LogicalDevice::GetPresentSemaphores() const noexcept
    {
	    return _presentSemaphores;
    }

    //-------------------------------------------------------------------------------------------------

    VkFormat LogicalDevice::GetDepthFormat() const noexcept
    {
	    return _depthFormat;
    }

    //-------------------------------------------------------------------------------------------------

    VkSurfaceFormatKHR LogicalDevice::GetSurfaceFormat() const noexcept
    {
	    return _surfaceFormat;
    }

    //-------------------------------------------------------------------------------------------------

    VkSemaphore LogicalDevice::GetComputeSemaphore(RT::CommandRecordState const& recordState) const noexcept
    {
	    return _computeSemaphores[recordState.frameIndex];
    }

    //-------------------------------------------------------------------------------------------------

    VkSemaphore LogicalDevice::GetPresentSemaphore(RT::CommandRecordState const& recordState) const noexcept
    {
	    return _presentSemaphores[recordState.frameIndex];
    }

    //-------------------------------------------------------------------------------------------------

    VkFence LogicalDevice::GetFence(RT::CommandRecordState const& recordState) const
    {
        return _fences[recordState.frameIndex];
    }

    //-------------------------------------------------------------------------------------------------

    void LogicalDevice::DeviceWaitIdle() const
    {
        RB::DeviceWaitIdle(_vkDevice);
    }

    //-------------------------------------------------------------------------------------------------

    SDL_Window *LogicalDevice::GetWindow() const noexcept { return _window; }

    //-------------------------------------------------------------------------------------------------

    void LogicalDevice::AddRenderTask(RenderTask renderTask)
    {
        MFA_ASSERT(renderTask != nullptr);
        if (Instance == nullptr)
        {
            MFA_LOG_WARN("Logical device does not exist, cannot add render task");
            return;
        }
        Instance->_renderTasks.Push(std::move(renderTask));
    }

    //-------------------------------------------------------------------------------------------------

    VkCommandBuffer LogicalDevice::GetComputeCommandBuffer(RT::CommandRecordState const& recordState) const
    {
        return _computeCommandBuffer->commandBuffers[recordState.frameIndex];
    }

    //-------------------------------------------------------------------------------------------------

    VkCommandBuffer LogicalDevice::GetGraphicCommandBuffer(RT::CommandRecordState const& recordState) const
    {
        return _graphicCommandBuffer->commandBuffers[recordState.frameIndex];
    }

    //-------------------------------------------------------------------------------------------------

    void LogicalDevice::BeginCommandBuffer(
        RT::CommandRecordState & recordState,
        RT::CommandBufferType commandBufferType,
        VkCommandBufferBeginInfo const & beginInfo
    )
    {
        MFA_ASSERT(recordState.isValid);

        VkCommandBuffer commandBuffer {};

        switch (commandBufferType)
        {
        case RT::CommandBufferType::Compute:
        {
            commandBuffer = GetComputeCommandBuffer(recordState);
        }
        break;
        case RT::CommandBufferType::Graphic:
        {
            commandBuffer = GetGraphicCommandBuffer(recordState);
        }
        break;
        default:
            MFA_ASSERT(false);
            break;
        }

        RB::BeginCommandBuffer(
            commandBuffer,
            beginInfo
        );

        MFA_ASSERT(recordState.isValid);
        MFA_ASSERT(recordState.commandBuffer == VK_NULL_HANDLE);
        MFA_ASSERT(commandBufferType != RT::CommandBufferType::Invalid);
        MFA_ASSERT(recordState.commandBufferType == RT::CommandBufferType::Invalid);

        recordState.commandBufferType = commandBufferType;
        recordState.commandBuffer = commandBuffer;

        {
            {
                int const count = (int)_pRenderTasks.size();
                for (int i = 0; i < count; i++)
                {
                    RenderTask task = _pRenderTasks.front();
                    _pRenderTasks.pop();
                    if (task(recordState) == true)
                    {
                        _pRenderTasks.push(task);
                    }
                }
            }
            {
                int const count = (int)_renderTasks.ItemCount();
                for (int i = 0; i < count; i++)
                {
                    RenderTask task;
                    bool isEmpty;
                    if (_renderTasks.TryToPop(task, isEmpty))
                    {
                        if (task(recordState) == true)
                        {
                            _pRenderTasks.emplace(task);
                        }
                    }
                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------------

    void LogicalDevice::EndCommandBuffer(RT::CommandRecordState & recordState)
    {
        MFA_ASSERT(recordState.isValid);
        MFA_ASSERT(recordState.commandBuffer != VK_NULL_HANDLE);
        MFA_ASSERT(recordState.commandBufferType != RT::CommandBufferType::Invalid);

        RB::EndCommandBuffer(recordState.commandBuffer);

        recordState.commandBufferHistory.emplace_back(recordState.commandBufferType);
        recordState.commandBuffer = nullptr;
        recordState.commandBufferType = RT::CommandBufferType::Invalid;
    }

    //-------------------------------------------------------------------------------------------------

    void LogicalDevice::SubmitQueues(RT::CommandRecordState & recordState)
    {
        const auto computeSemaphore = GetComputeSemaphore(recordState);
        const auto presentSemaphore = GetPresentSemaphore(recordState);

        bool hasGraphicSubmission = false;
        bool hasComputeSubmission = false;

        for (auto & history : recordState.commandBufferHistory)
        {
            if (history == RT::CommandBufferType::Graphic)
            {
                hasGraphicSubmission = true;
            } else if (history == RT::CommandBufferType::Compute)
            {
                hasComputeSubmission = true;
            }
        }

        if (hasComputeSubmission)
        {
            // Submit compute queue
            VkPipelineStageFlags computeWaitStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

            std::vector<VkSemaphore> computeSignalSemaphores{ computeSemaphore };

            auto computeCommandBuffer = GetComputeCommandBuffer(recordState);

            VkSubmitInfo const submitInfo{
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                .waitSemaphoreCount = 0,
                .pWaitSemaphores = nullptr,
                .pWaitDstStageMask = &computeWaitStageMask,
                .commandBufferCount = 1,
                .pCommandBuffers = &computeCommandBuffer,
                .signalSemaphoreCount = static_cast<uint32_t>(computeSignalSemaphores.size()),
                .pSignalSemaphores = computeSignalSemaphores.data(),
            };

            RB::SubmitQueues(
                _computeQueue,
                1,
                &submitInfo,
                nullptr
            );
        }

        if (hasGraphicSubmission)
        {
            // Submit graphic queue
            std::vector<VkSemaphore> graphicWaitSemaphores{presentSemaphore};

            if (hasComputeSubmission == true)
            {
                graphicWaitSemaphores.emplace_back(computeSemaphore);
            }

            std::vector<VkPipelineStageFlags> graphicWaitDstStageMask{
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_PIPELINE_STAGE_VERTEX_INPUT_BIT
            };

            std::vector<VkSemaphore> graphicSignalSemaphores{};

            auto graphicCommandBuffer = GetGraphicCommandBuffer(recordState);

            VkSubmitInfo const submitInfo {
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                .waitSemaphoreCount = static_cast<uint32_t>(graphicWaitSemaphores.size()),
                .pWaitSemaphores = graphicWaitSemaphores.data(),
                .pWaitDstStageMask = graphicWaitDstStageMask.data(),
                .commandBufferCount = 1,
                .pCommandBuffers = &graphicCommandBuffer,
                .signalSemaphoreCount = static_cast<uint32_t>(graphicSignalSemaphores.size()),
                .pSignalSemaphores = graphicSignalSemaphores.data(),
            };

            RB::SubmitQueues(
                _graphicQueue,
                1,
                &submitInfo,
                GetFence(recordState)
            );
        }
    }

    //-------------------------------------------------------------------------------------------------

    void LogicalDevice::Present(RT::CommandRecordState const & recordState, VkSwapchainKHR swapChain)
    {
        // Present drawn image
        // Note: semaphore here is not strictly necessary, because commands are processed in submission order within a single queue
        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 0;
        presentInfo.pWaitSemaphores = nullptr;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapChain;
        presentInfo.pImageIndices = &recordState.imageIndex;

        auto const res = vkQueuePresentKHR(_presentQueue, &presentInfo);
        if (res == VK_SUBOPTIMAL_KHR || res == VK_ERROR_OUT_OF_DATE_KHR || _windowResized == true)
        {
            DeviceWaitIdle();
            _windowResized = true;
        }
        else if (res != VK_SUCCESS)
        {
            MFA_CRASH("Failed to submit present command buffer");
        }
    }

    //-------------------------------------------------------------------------------------------------

}
