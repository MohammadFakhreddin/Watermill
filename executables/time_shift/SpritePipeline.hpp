#pragma once

#include "render_pass/DisplayRenderPass.hpp"
#include "renderer/IShadingPipeline.hpp"

#include <glm/glm.hpp>

#include <atomic>

class SpritePipeline : public MFA::IShadingPipeline
{
public:

    using Position = glm::vec3;
    using UV = glm::vec2;
    using Color = glm::vec4;
    using Index = uint16_t;

    struct Vertex
    {
        Position position{};
        UV uv{};
    };

    struct PushConstants
    {
        glm::vec4 color{};
        glm::mat4 model{};
        glm::mat4 viewProjection{};
    };

    explicit SpritePipeline(
        std::shared_ptr<MFA::DisplayRenderPass> displayRenderPass,
        std::shared_ptr<MFA::RT::SamplerGroup> sampler
    );

    ~SpritePipeline();

    [[nodiscard]]
    bool IsBinded(MFA::RT::CommandRecordState const &recordState) const;

    void BindPipeline(MFA::RT::CommandRecordState &recordState) const;

    [[nodiscard]]
    MFA::RT::DescriptorSetGroup CreateDescriptorSet(
        int count,
        MFA::RT::GpuTexture const &texture
    );

    void UpdateDescriptorSet(
        int frameIndex,
        MFA::RT::DescriptorSetGroup &descriptorSetGroup,
        MFA::RT::GpuTexture const &texture
    ) const;

    void FreeDescriptorSet(MFA::RT::DescriptorSetGroup &descriptorSetGroup);

    void SetPushConstant(MFA::RT::CommandRecordState &recordState, PushConstants const &pushConstant) const;

    void Reload() override;

private:

    void CreateDescriptorLayout();

    void CreatePipeline();

    std::shared_ptr<MFA::DisplayRenderPass> _displayRenderPass{};

    std::shared_ptr<MFA::RT::SamplerGroup> _sampler{};

    std::atomic<bool> _descriptorPoolLock{};
    std::shared_ptr<MFA::RT::DescriptorPool> _descriptorPool{};

    std::shared_ptr<MFA::RT::DescriptorSetLayoutGroup> _descriptorLayout{};

    std::shared_ptr<MFA::RT::PipelineGroup> _pipeline{};

    VkShaderStageFlags _pushConstantsStageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
};