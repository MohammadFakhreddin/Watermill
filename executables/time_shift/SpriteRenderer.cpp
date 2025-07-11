#include "SpriteRenderer.hpp"
#include "LogicalDevice.hpp"
#include "Time.hpp"

#include <utility>

using namespace MFA;

//------------------------------------------------------------------------------------------------------------------

SpriteRenderer::SpriteRenderer(std::shared_ptr<Pipeline> pipeline)
    : _pipeline(std::move(pipeline))
{}

//------------------------------------------------------------------------------------------------------------------

std::tuple<std::shared_ptr<MFA::RenderTypes::BufferAndMemory>, std::shared_ptr<MFA::RT::BufferGroup>>
SpriteRenderer::AllocateVertexBuffer(
    VkCommandBuffer commandBuffer,
    int vertexCount,
    Position const *vertices,
    UV const *uvs
)
{
    auto * device = LogicalDevice::Instance;

    std::vector<Pipeline::Vertex> vertexDataList{};
    for (int i = 0; i < vertexCount; ++i)
    {
        vertexDataList.emplace_back();
        auto & vertexData = vertexDataList.back();
        vertexData.position = vertices[i];
        vertexData.uv = uvs[i];
    }

    auto const vertexAlias = Alias(vertexDataList.data(), vertexDataList.size());

    auto vertexStageBuffer = RB::CreateStageBuffer(
        device->GetVkDevice(),
        device->GetPhysicalDevice(),
        vertexAlias.Len(),
        1
    );

    auto vertexBuffer = RB::CreateVertexBuffer(
        device->GetVkDevice(),
        device->GetPhysicalDevice(),
        commandBuffer,
        *vertexStageBuffer->buffers[0],
        vertexAlias
    );

    return {vertexBuffer, vertexStageBuffer};
}

//------------------------------------------------------------------------------------------------------------------

std::tuple<std::shared_ptr<MFA::RT::BufferAndMemory>, std::shared_ptr<MFA::RT::BufferGroup>>
SpriteRenderer::AllocateIndexBuffer(
    VkCommandBuffer commandBuffer,
    int const indexCount,
    Index const * indices
)
{
    auto *device = LogicalDevice::Instance;

    auto const indexAlias = Alias(indices, indexCount);

    auto indexStageBuffer = RB::CreateStageBuffer(
        device->GetVkDevice(),
        device->GetPhysicalDevice(),
        indexAlias.Len(),
        1
    );

    auto indexBuffer = RB::CreateIndexBuffer(
        device->GetVkDevice(),
        device->GetPhysicalDevice(),
        commandBuffer,
        *indexStageBuffer->buffers[0],
        indexAlias
    );

    return {indexBuffer, indexStageBuffer};
}

//------------------------------------------------------------------------------------------------------------------

std::shared_ptr<SpriteRenderer::Material> SpriteRenderer::AllocateMaterial(std::shared_ptr<RT::GpuTexture> albedo) const
{
    MFA_ASSERT(albedo != nullptr);

    int frameCount = LogicalDevice::Instance->GetMaxFramePerFlight();
    std::vector<std::shared_ptr<MFA::RT::GpuTexture>> albedoList (frameCount);
    for (int i = 0; i < frameCount; ++i)
    {
        albedoList[i] = albedo;
    }

    auto material = std::make_shared<Material>(Material{
        .descriptorSet = _pipeline->CreateDescriptorSet(frameCount, *albedo),
        .albedoList = albedoList
    });
    return material;
}

//------------------------------------------------------------------------------------------------------------------

void SpriteRenderer::UpdateMaterial(
    int const frameIndex,
    Material & material,
    std::shared_ptr<MFA::RT::GpuTexture> albedo
) const
{
    MFA_ASSERT(albedo != nullptr);
    _pipeline->UpdateDescriptorSet(frameIndex, material.descriptorSet, *albedo);
    material.albedoList[frameIndex] = std::move(albedo);
}

//------------------------------------------------------------------------------------------------------------------

std::unique_ptr<SpriteRenderer::Sprite> SpriteRenderer::CreateSprite(
    std::shared_ptr<RT::BufferAndMemory> vertexBuffer,
    int indexCount,
    std::shared_ptr<RT::BufferAndMemory> indexBuffer,
    std::shared_ptr<Material> material
)
{
    auto imageData = std::make_unique<Sprite>(Sprite{
        .vertexData = std::move(vertexBuffer),
        .indexCount = (size_t)indexCount,
        .indexData = std::move(indexBuffer),
        .material = std::move(material),
    });
    return imageData;
}

//------------------------------------------------------------------------------------------------------------------

void SpriteRenderer::Draw(
    RT::CommandRecordState &recordState,
    Pipeline::PushConstants const &pushConstants,
    Sprite const &imageData
) const
{
    _pipeline->BindPipeline(recordState);

    _pipeline->SetPushConstant(recordState, pushConstants);

    RB::AutoBindDescriptorSet(recordState, RB::UpdateFrequency::PerPipeline, imageData.material->descriptorSet);

    RB::BindVertexBuffer(recordState, *imageData.vertexData);
    RB::BindIndexBuffer(recordState, *imageData.indexData, 0, RB::GetVkIndexType(sizeof(Index)));
    vkCmdDrawIndexed(recordState.commandBuffer, imageData.indexCount, 1, 0, 0, 0);
}

//------------------------------------------------------------------------------------------------------------------

