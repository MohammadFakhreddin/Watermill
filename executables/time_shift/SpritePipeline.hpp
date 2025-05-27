#pragma once

#include "render_pass/DisplayRenderPass.hpp"
#include "renderer/IShadingPipeline.hpp"

#include <glm/glm.hpp>

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
        Color color{};
    };

    struct PushConstants
    {
        glm::mat4 model{};
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
    MFA::RT::DescriptorSetGroup CreateDescriptorSet(MFA::RT::GpuTexture const &texture) const;

    void UpdateDescriptorSet(MFA::RT::DescriptorSetGroup &descriptorSetGroup, MFA::RT::GpuTexture const &texture) const;

    void FreeDescriptorSet(MFA::RT::DescriptorSetGroup &descriptorSetGroup) const;

    void SetPushConstant(MFA::RT::CommandRecordState &recordState, PushConstants const &pushConstant) const;

    void Reload() override;

private:

    void CreateDescriptorLayout();

    void CreatePipeline();

    std::shared_ptr<MFA::DisplayRenderPass> _displayRenderPass{};

    std::shared_ptr<MFA::RT::SamplerGroup> _sampler{};

    std::shared_ptr<MFA::RT::DescriptorPool> _descriptorPool{};

    std::shared_ptr<MFA::RT::DescriptorSetLayoutGroup> _descriptorLayout{};

    std::shared_ptr<MFA::RT::PipelineGroup> _pipeline{};
};