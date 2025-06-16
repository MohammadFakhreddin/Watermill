#include "ResourceManager.hpp"

#include "BedrockPath.hpp"
#include "LogicalDevice.hpp"
#include "ScopeLock.hpp"
#include "RenderTypes.hpp"
#include "JobSystem.hpp"
#include "ImportTexture.hpp"

using namespace MFA;

//======================================================================================================================

std::shared_ptr<ResourceManager> ResourceManager::Instance()
{
    std::shared_ptr<ResourceManager> shared_ptr = _instance.lock();
    if (shared_ptr == nullptr)
    {
        shared_ptr = std::make_shared<ResourceManager>();
        _instance = shared_ptr;
    }
    return shared_ptr;
}

//======================================================================================================================

ResourceManager::~ResourceManager() = default;

//======================================================================================================================

void ResourceManager::RequestImage(char const * name_, const ImageCallback & callback, bool const forceReload)
{
    auto & [lock, imageWeak] = _imageMap[name_];

    MFA_SCOPE_LOCK(lock);

    if (forceReload == false)
    {
        auto imageShared = imageWeak.lock();
        if (imageShared != nullptr)
        {
            callback(imageShared);
        }
    }

    _imageCallbacks[name_].Push(callback);
    // To make sure we are not requesting things multiple times
    if (_imageCallbacks[name_].ItemCount() != 1)
    {
        return;
    }
    // TODO: In the new design, our functions should return a command buffer that they have allocated themselves and we just combine them together for execution
    JobSystem::Instance()->AssignTask([this, name = std::string(name_)]()
    {
        auto * device = LogicalDevice::Instance->GetVkDevice();
        auto * physicalDevice = LogicalDevice::Instance->GetPhysicalDevice();

        auto const path = Path::Instance()->Get(name);
        auto const cpuTexture = Importer::UncompressedImage(path);

        auto const buffer = cpuTexture->GetBuffer();
        MFA_ASSERT(buffer != nullptr && buffer->IsValid() == true);

        // TODO: Memory pool to reuse upload buffer groups
        // Create upload buffer
        auto const uploadBufferGroup = RB::CreateBuffer(    // TODO: We can cache this buffer
            device,
            physicalDevice,
            buffer->Len(),
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );

        // Map texture data to buffer
        RB::CopyDataToHostVisibleBuffer(device, uploadBufferGroup->memory, *buffer);

        if (_instance.lock() == nullptr)
        {
            return;
        }

        _stageBufferMap[name] = uploadBufferGroup;
        _cpuTextureMap[name] = cpuTexture;

        _nextUpdateTasks.Push([this, name](const RT::CommandRecordState & recordState)
        {
            if (_cpuTextureMap.contains(name) == false || _stageBufferMap.contains(name) == false)
            {
                return;
            }
            auto cpuTexture = _cpuTextureMap[name];
            auto stageBuffer = _stageBufferMap[name];

            auto * device = LogicalDevice::Instance->GetVkDevice();
            auto * physicalDevice = LogicalDevice::Instance->GetPhysicalDevice();

            auto const format = cpuTexture->GetFormat();
            auto const mipCount = cpuTexture->GetMipCount();
            auto const sliceCount = cpuTexture->GetSlices();
            auto const& largestMipmapInfo = cpuTexture->GetMipmap(0);

            auto & [lock, imageWeak] = _imageMap[name];
            MFA_SCOPE_LOCK(lock);

            auto const vulkan_format = RB::ConvertCpuTextureFormatToGpu(format);

            auto imageGroup = RB::CreateImage(
                device,
                physicalDevice,
                largestMipmapInfo.dimension.width,
                largestMipmapInfo.dimension.height,
                static_cast<uint32_t>(largestMipmapInfo.dimension.depth),
                mipCount,
                sliceCount,
                vulkan_format,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_SAMPLE_COUNT_1_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            );

            RB::TransferImageLayout(
                device,
                recordState.commandBuffer,
                imageGroup->image,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                mipCount,
                sliceCount
            );

            RB::CopyBufferToImage(
                device,
                recordState.commandBuffer,
                stageBuffer->buffer,
                imageGroup->image,
                *cpuTexture
            );

            RB::TransferImageLayout(
                device,
                recordState.commandBuffer,
                imageGroup->image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                mipCount,
                sliceCount
            );

            auto imageView = RB::CreateImageView(
                device,
                imageGroup->image,
                vulkan_format,
                VK_IMAGE_ASPECT_COLOR_BIT,
                mipCount,
                sliceCount,
                VK_IMAGE_VIEW_TYPE_2D
            );

            std::shared_ptr<RT::GpuTexture> gpuTexture = std::make_shared<RT::GpuTexture>(
                imageGroup,
                imageView
            );

            _activeBuffers.emplace_back();
            auto & item = _activeBuffers.back();
            item.gpuTexture = gpuTexture;
            item.stageBuffer = stageBuffer;
            item.lifeTime = (int)LogicalDevice::Instance->GetMaxFramePerFlight() + 1;

            imageWeak = gpuTexture;

            auto & imageCallbacks = _imageCallbacks[name];
            while (imageCallbacks.IsEmpty() == false)
            {
                auto callback = imageCallbacks.Pop();
                callback(gpuTexture);
            }

            _imageCallbacks.erase(name);
            _stageBufferMap.erase(name);
            _cpuTextureMap.erase(name);
        });
    });
}

//======================================================================================================================

// std::future<std::shared_ptr<Blob>> ResourceManager::RequestBlob(char const * name, bool forceReload)
// {
//
// }

//======================================================================================================================

void ResourceManager::UpdateBuffers(RT::CommandRecordState & recordState)
{
    while (_nextUpdateTasks.IsEmpty() == false)
    {
        auto task = _nextUpdateTasks.Pop();
        task(recordState);
    }

    for (int i = (int)_activeBuffers.size() - 1; i >= 0; i--)
    {
        auto & item = _activeBuffers[i];
        item.lifeTime -= 1;
        if (item.lifeTime <= 0)
        {
            _activeBuffers.erase(_activeBuffers.begin() + i);
        }
    }
}

//======================================================================================================================

void ResourceManager::ForceCleanUp()
{
    while (_nextUpdateTasks.IsEmpty() == false)
    {
        _nextUpdateTasks.Pop();
    }

    _imageMap.clear();
    _imageCallbacks.clear();
    _stageBufferMap.clear();
    _cpuTextureMap.clear();
}

//======================================================================================================================
