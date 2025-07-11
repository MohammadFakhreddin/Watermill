#include "ResourceManager.hpp"

#include "BedrockPath.hpp"
#include "ImportTexture.hpp"
#include "JobSystem.hpp"
#include "LogicalDevice.hpp"
#include "RenderTypes.hpp"
#include "ScopeLock.hpp"
#include "Time.hpp"

using namespace MFA;

//======================================================================================================================

std::shared_ptr<ResourceManager> ResourceManager::Instance(bool const createNewIfNotExists)
{
    std::shared_ptr<ResourceManager> shared_ptr = _instance.lock();
    if (shared_ptr == nullptr && createNewIfNotExists == true)
    {
        shared_ptr = std::make_shared<ResourceManager>();
        _instance = shared_ptr;
    }
    return shared_ptr;
}

//======================================================================================================================

ResourceManager::ResourceManager()
{
    // TODO: The other approach should have worked as well
    // LogicalDevice::Instance->AddRenderTask([](RT::CommandRecordState & recordState)->bool
    // {
    auto * logicalDevice = LogicalDevice::Instance;
    auto * device = logicalDevice->GetVkDevice();
    auto * physicalDevice = logicalDevice->GetPhysicalDevice();
    auto * commandPool = logicalDevice->GetGraphicCommandPool();
    auto * graphicQueue = logicalDevice->GetGraphicQueue();

    auto commandBuffer = RB::BeginSingleTimeCommand(device, *commandPool);

    auto cpuTexture = Importer::ErrorTexture();
    std::vector<uint8_t> mipLevel{0};
    auto [gpuTexture, stageBuffer] = RB::CreateTexture(
        *cpuTexture,
        device,
        physicalDevice,
        // recordState.commandBuffer,
        commandBuffer,
        (int)mipLevel.size(),
        mipLevel.data()
    );

    _errorTexture = std::move(gpuTexture);

    RB::EndAndSubmitSingleTimeCommand(device, *commandPool, graphicQueue, commandBuffer);
        // return false;
    // });
}

//======================================================================================================================

ResourceManager::~ResourceManager() = default;

//======================================================================================================================

void ResourceManager::RequestImage(char const * nameRaw_, const ImageCallback & callback)
{

    std::string imagePath{nameRaw_};

    auto & lock = _lockMap[imagePath];
    MFA_SCOPE_LOCK(lock);

    auto & imageData = _imageMap[imagePath];
    if (imageData == nullptr)
    {
        imageData = std::make_shared<ImageData>();
        imageData->path = imagePath;
        imageData->currentMipLevel = -1;

        std::filesystem::path originalPath{imagePath};
        std::filesystem::path parentPath = originalPath.parent_path().string();
        std::filesystem::path stem = originalPath.stem().string();
        std::string extension = originalPath.extension().string();

        int mipIdx = 0;
        while(true)
        {
            auto mipPath = (parentPath / stem).string() + "_mip_" + std::to_string(mipIdx) + extension;
            if (std::filesystem::exists(mipPath) == false)
            {
                break;
            }
            ++mipIdx;
        }

        imageData->mipCount = mipIdx;
        imageData->hasMipmaps = mipIdx > 0;
        imageData->currentMipLevel = imageData->mipCount;
    }
    else
    {
        auto imageShared = imageData->gpuTexture.lock();
        if (imageShared != nullptr)
        {
            callback(imageShared);

            // We have loaded the highest mip level already
            if (imageData->hasMipmaps == false || imageData->currentMipLevel == 0)
            {
                return;
            }
        }
    }

    imageData->callbacks.Push(callback);
    // To make sure we are not requesting things multiple times
    if (imageData->callbacks.ItemCount() != 1)
    {
        return;
    }

    imageData->currentMipLevel = imageData->mipCount;
    RequestNextMipmap(*imageData, imageData->currentMipLevel - 1);
}

//======================================================================================================================

void ResourceManager::ForceCleanUp()
{
    _imageMap.clear();
    _lockMap.clear();
}

//======================================================================================================================

void ResourceManager::RequestNextMipmap(ImageData &imageData, int nextMipLevel)
{
    MFA_ASSERT(nextMipLevel >= 0);

    std::string imagePath{imageData.path.string()};
    if (imageData.hasMipmaps == true)
    {
        std::filesystem::path originalPath{imagePath};
        std::filesystem::path parentPath = originalPath.parent_path().string();
        std::filesystem::path stem = originalPath.stem().string();
        std::string extension = originalPath.extension().string();
        auto mipPath = (parentPath / stem).string() + "_mip_" + std::to_string(nextMipLevel) + originalPath.extension().string();

        imagePath = mipPath;
    }

    std::weak_ptr rcWeak = shared_from_this();
    // TODO: In the new design, our functions should return a command buffer that they have allocated themselves and we just combine them together for execution
    JobSystem::Instance()->AssignTask([rcWeak, name = std::string(imagePath), &imageData, nextMipLevel]()
    {
        if (rcWeak.lock() == nullptr)
        {
            return;
        }

        auto * logicalDevice = LogicalDevice::Instance;
        auto * device = logicalDevice->GetVkDevice();
        auto * commandPool = logicalDevice->GetGraphicCommandPool();
        auto * physicalDevice = logicalDevice->GetPhysicalDevice();

        auto commandBufferGroup = RB::BeginSecondaryCommand(
            device,
            *commandPool
        );

        auto commandBuffer = commandBufferGroup->commandBuffers[0];

        std::shared_ptr<Asset::Texture> cpuTexture;

        std::filesystem::path const path = Path::Instance()->Get(name);
        if (path.extension() == ".ktx2")
        {
            cpuTexture = Importer::LoadKtxMetadata(path.string().c_str());
        }
        else
        {
            cpuTexture = Importer::UncompressedImage(path.string());
        }

        auto const buffer = cpuTexture->GetMipmapBuffer(0);
        if (buffer == nullptr || buffer->IsValid() == false)
        {
            MFA_LOG_WARN("Failed to load image: %s", name.c_str());
            return;
        }
        // MFA_ASSERT(buffer != nullptr && buffer->IsValid() == true);

        std::vector<uint8_t> mipLevel{0};
        auto [gpuTexture, stageBuffer] = RB::CreateTexture(
            *cpuTexture,
            device,
            physicalDevice,
            commandBuffer,
            mipLevel.size(),
            mipLevel.data()
        );

        RB::EndCommandBuffer(commandBuffer);

        auto const instance = _instance.lock();
        if (instance == nullptr)
        {
            return;
        }

        struct QueuedImages
        {
            std::shared_ptr<RenderTypes::GpuTexture> gpuTexture;
            std::shared_ptr<RenderTypes::BufferAndMemory> stageBuffer;
            std::shared_ptr<RenderTypes::CommandBufferGroup> commandBufferGroup;
            int lifeTime = 0;
        };
        std::shared_ptr imageBuffer = std::make_shared<QueuedImages>(QueuedImages{
            .gpuTexture = gpuTexture,
            .stageBuffer = stageBuffer,
            .commandBufferGroup = commandBufferGroup,
            .lifeTime = (int)LogicalDevice::Instance->GetMaxFramePerFlight() + 1
        });
        Time::AddUpdateTask([imageBuffer]()->bool
        {
            imageBuffer->lifeTime -= 1;
            if (imageBuffer->lifeTime < 0)
            {
                return false;
            }
            return true;
        });

        imageData.gpuTexture = gpuTexture;
        imageData.currentMipLevel = nextMipLevel;

        LogicalDevice::Instance->AddRenderTask([
            rcWeak,
            name = std::move(name),
            commandBuffer,
            &imageData
        ](const RT::CommandRecordState & recordState)->bool
        {
            MFA_ASSERT(JobSystem::Instance() == nullptr || JobSystem::Instance()->IsMainThread() == true);

            auto rc = rcWeak.lock();
            if (rc == nullptr)
            {
                return false;
            }

            vkCmdExecuteCommands(recordState.commandBuffer, 1, &commandBuffer);

            auto & lock = rc->_lockMap[imageData.path.string()];
            MFA_SCOPE_LOCK(lock);

            auto gpuTexture = imageData.gpuTexture.lock();
            if (gpuTexture == nullptr)
            {
                return false;
            }

            for (int i = 0; i < imageData.callbacks2.size(); ++i)
            {
                auto & callback = imageData.callbacks2[i];
                callback(gpuTexture);
            }

            auto & imageCallbacks = imageData.callbacks;
            while (imageCallbacks.IsEmpty() == false)
            {
                auto callback = imageCallbacks.Pop();
                callback(gpuTexture);
                imageData.callbacks2.emplace_back(callback);
            }

            if (imageData.hasMipmaps == true && imageData.currentMipLevel > 0)
            {
                rc->RequestNextMipmap(imageData, imageData.currentMipLevel - 1);
            }
            else
            {
                imageData.callbacks2.clear();
            }

            return false;
        });
    });
}

//======================================================================================================================
