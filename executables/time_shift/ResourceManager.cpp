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

ResourceManager::~ResourceManager() = default;

//======================================================================================================================

void ResourceManager::RequestImage(char const * nameRaw_, const ImageCallback & callback, bool const forceReload)
{

    std::string imagePath{nameRaw_};

    std::filesystem::path originalPath{imagePath};
    // std::filesystem::path ktx2Path = originalPath;
    // ktx2Path.replace_extension(".ktx2");
    //
    // if (std::filesystem::exists(ktx2Path)) {
    //     imagePath = ktx2Path.string();
    // }

    auto & [lock, imageWeak] = _imageMap[imagePath];

    MFA_SCOPE_LOCK(lock);

    if (forceReload == false)
    {
        auto imageShared = imageWeak.lock();
        if (imageShared != nullptr)
        {
            callback(imageShared);
        }
    }

    _imageCallbacks[imagePath].Push(callback);
    // To make sure we are not requesting things multiple times
    if (_imageCallbacks[imagePath].ItemCount() != 1)
    {
        return;
    }

    std::weak_ptr rcWeak = shared_from_this();
    // TODO: In the new design, our functions should return a command buffer that they have allocated themselves and we just combine them together for execution
    JobSystem::Instance()->AssignTask([rcWeak, name = std::string(imagePath)]()
    {
        if (rcWeak.lock() == nullptr)
        {
            return;
        }

        auto * logicalDevice = LogicalDevice::Instance;
        auto * device = logicalDevice->GetVkDevice();
        auto * commandPool = logicalDevice->GetGraphicCommandPool();
        auto * physicalDevice = logicalDevice->GetPhysicalDevice();
        MFA_SCOPE_LOCK(commandPool->lock);

        auto commandBufferGroup = RB::BeginSecondaryCommand(
            device,
            *commandPool
        );

        auto commandBuffer = commandBufferGroup->commandBuffers[0];

        std::shared_ptr<Asset::Texture> cpuTexture;

        std::filesystem::path const path = Path::Instance()->Get(name);
        if (path.extension() == ".ktx2")
        {
            cpuTexture = Importer::LoadKtxMetadata(path.c_str());
        }
        else
        {
            cpuTexture = Importer::UncompressedImage(path);
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

        auto & [lock, imageWeak] = instance->_imageMap[name];
        MFA_SCOPE_LOCK(lock);

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

        imageWeak = gpuTexture;

        LogicalDevice::Instance->AddRenderTask([
            rcWeak,
            name = std::move(name),
            commandBuffer
        ](const RT::CommandRecordState & recordState)->bool
        {
            MFA_ASSERT(JobSystem::Instance() == nullptr || JobSystem::Instance()->IsMainThread() == true);

            auto rc = rcWeak.lock();
            if (rc == nullptr)
            {
                return false;
            }

            vkCmdExecuteCommands(recordState.commandBuffer, 1, &commandBuffer);

            auto & [lock, imageWeak] = rc->_imageMap[name];
            auto gpuTexture = imageWeak.lock();
            if (gpuTexture != nullptr)
            {
                // I think it is better to call these callback from the main thread.
                auto & imageCallbacks = rc->_imageCallbacks[name];
                while (imageCallbacks.IsEmpty() == false)
                {
                    auto callback = imageCallbacks.Pop();
                    callback(gpuTexture);
                }
            }

            rc->_imageCallbacks.erase(name);

            return false;
        });
    });
}

//======================================================================================================================

void ResourceManager::ForceCleanUp()
{
    _imageMap.clear();
    _imageCallbacks.clear();
}

//======================================================================================================================
