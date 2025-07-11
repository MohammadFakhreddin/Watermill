#pragma once

#include "SpritePipeline.hpp"
#include "BufferTracker.hpp"

#include <memory>

class SpriteRenderer
{
public:

    using Pipeline = SpritePipeline;
    using Position = Pipeline::Position;
    using UV = Pipeline::UV;
    using Color = Pipeline::Color;
    using Index = Pipeline::Index;

    struct Material
    {
        MFA::RT::DescriptorSetGroup descriptorSet {};
        std::vector<std::shared_ptr<MFA::RT::GpuTexture>> albedoList {};
    };

    // TODO: This can be optimized like uvs might be per instance but vertices are per vertex
    struct Sprite
    {
        std::shared_ptr<MFA::RT::BufferAndMemory> vertexData {};
        size_t indexCount{};
        std::shared_ptr<MFA::RT::BufferAndMemory> indexData {};
        std::shared_ptr<Material> material {};
    };

    explicit SpriteRenderer(std::shared_ptr<Pipeline> pipeline);

    static std::tuple<std::shared_ptr<MFA::RenderTypes::BufferAndMemory>, std::shared_ptr<MFA::RT::BufferGroup>>
    AllocateVertexBuffer(
        VkCommandBuffer commandBuffer,
        int vertexCount,
        Position const * vertices,
        UV const * uvs
    );

    static std::tuple<std::shared_ptr<MFA::RT::BufferAndMemory>, std::shared_ptr<MFA::RT::BufferGroup>>
    AllocateIndexBuffer(
        VkCommandBuffer commandBuffer,
        int indexCount,
        Index const * indicese
    );

    [[nodiscard]]
    std::shared_ptr<Material> AllocateMaterial(std::shared_ptr<MFA::RT::GpuTexture> albedo) const;

    void UpdateMaterial(
        int frameIndex,
        Material & material,
        std::shared_ptr<MFA::RT::GpuTexture> albedo
    ) const;

    [[nodiscard]]
    static std::unique_ptr<Sprite> CreateSprite(
        std::shared_ptr<MFA::RT::BufferAndMemory> vertexBuffer,
        int indexCount,
        std::shared_ptr<MFA::RT::BufferAndMemory> indexBuffer,
        std::shared_ptr<Material> material
    );

    void Draw(
        MFA::RT::CommandRecordState & recordState,
        Pipeline::PushConstants const & pushConstants,
        Sprite const & imageData
    ) const;

private:

    std::shared_ptr<Pipeline> _pipeline;

};
