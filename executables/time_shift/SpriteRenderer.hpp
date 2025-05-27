#pragma once

#include "SpritePipeline.hpp"

#include <memory>

#include "BufferTracker.hpp"

class SpriteRenderer
{
public:

    using Pipeline = SpritePipeline;
    using Position = Pipeline::Position;
    using UV = Pipeline::UV;
    using Color = Pipeline::Color;
    using Index = Pipeline::Index;
    // TODO: This can be optimized like uvs might be per instance but vertices are per vertex
    struct SpriteData
    {
        std::shared_ptr<MFA::RT::BufferAndMemory> vertexData {};
        size_t indexCount{};
        std::shared_ptr<MFA::RT::BufferAndMemory> indexData {};
        MFA::RT::DescriptorSetGroup descriptorSet {};
    };

    struct CommandBufferData
    {
        std::shared_ptr<MFA::RT::BufferGroup> vertexStageBuffer{};
        std::shared_ptr<MFA::RT::BufferGroup> indexStageBuffer{};
    };

    explicit SpriteRenderer(std::shared_ptr<Pipeline> pipeline);

    // Assumption is that sprites are not changing that frequently
    [[nodiscard]]
    std::tuple<std::unique_ptr<SpriteData>, std::unique_ptr<CommandBufferData>> Allocate(
        VkCommandBuffer commandBuffer,

        MFA::RT::GpuTexture const & gpuTexture,

        int vertexCount,
        Position const * vertices,
        UV const * uvs,
        Color color,

        int indexCount,
        Index const * indices
    ) const;

    // void FreeImageData(SpriteData &imageData);

    void Draw(
        MFA::RT::CommandRecordState & recordState,
        Pipeline::PushConstants const & pushConstants,
        SpriteData const & imageData
    ) const;

private:

    std::shared_ptr<Pipeline> _pipeline;

};
