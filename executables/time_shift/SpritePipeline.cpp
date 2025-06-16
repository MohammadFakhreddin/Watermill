#include "SpritePipeline.hpp"

#include "BedrockPath.hpp"
#include "DescriptorSetSchema.hpp"
#include "ImportShader.hpp"
#include "LogicalDevice.hpp"

using namespace MFA;

//======================================================================================================================
// TODO: There is alot of room for optimization
SpritePipeline::SpritePipeline(
    std::shared_ptr<MFA::DisplayRenderPass> displayRenderPass,
    std::shared_ptr<MFA::RT::SamplerGroup> sampler
)
    : _displayRenderPass(std::move(displayRenderPass))
    , _sampler(std::move(sampler))
{
    _descriptorPool = RB::CreateDescriptorPool(LogicalDevice::Instance->GetVkDevice(), 10000, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);
    CreateDescriptorLayout();
    CreatePipeline();
}

//======================================================================================================================

SpritePipeline::~SpritePipeline()
{
    _pipeline = nullptr;
    _descriptorLayout = nullptr;
    _descriptorPool = nullptr;
}

//======================================================================================================================

bool SpritePipeline::IsBinded(MFA::RT::CommandRecordState const &recordState) const
{
    if (recordState.pipeline == _pipeline.get())
    {
        return true;
    }
    return false;
}

//======================================================================================================================

void SpritePipeline::BindPipeline(MFA::RT::CommandRecordState &recordState) const
{
    if (IsBinded(recordState))
    {
        return;
    }

    RB::BindPipeline(recordState, *_pipeline);
}

//======================================================================================================================

MFA::RT::DescriptorSetGroup SpritePipeline::CreateDescriptorSet(MFA::RT::GpuTexture const &texture) const
{
    auto descriptorSetGroup = RB::CreateDescriptorSet(
        LogicalDevice::Instance->GetVkDevice(),
        _descriptorPool->descriptorPool,
        _descriptorLayout->descriptorSetLayout,
        1
    );

    UpdateDescriptorSet(descriptorSetGroup, texture);

    return descriptorSetGroup;
}

//======================================================================================================================

void SpritePipeline::UpdateDescriptorSet(
    MFA::RT::DescriptorSetGroup &descriptorSetGroup,
    MFA::RT::GpuTexture const &texture
) const
{
    MFA_ASSERT(descriptorSetGroup.descriptorSets.size() == 1);

    auto const &descriptorSet = descriptorSetGroup.descriptorSets[0];
    MFA_ASSERT(descriptorSet != VK_NULL_HANDLE);

    DescriptorSetSchema schema{descriptorSet};

    VkDescriptorImageInfo info{.sampler = _sampler->sampler,
                               .imageView = texture.imageView->imageView,
                               .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    schema.AddCombinedImageSampler(&info);

    schema.UpdateDescriptorSets();
}

//======================================================================================================================

void SpritePipeline::FreeDescriptorSet(MFA::RT::DescriptorSetGroup &descriptorSetGroup) const
{
    auto *device = LogicalDevice::Instance;
    vkFreeDescriptorSets(
        device->GetVkDevice(),
        _descriptorPool->descriptorPool,
        descriptorSetGroup.descriptorSets.size(),
        descriptorSetGroup.descriptorSets.data()
    );
}

//======================================================================================================================

void SpritePipeline::SetPushConstant(MFA::RT::CommandRecordState &recordState, PushConstants const &pushConstant) const
{
    RB::PushConstants(
        recordState,
        _pipeline->pipelineLayout,
        _pushConstantsStageFlags,
        0, Alias{pushConstant}
    );
}

//======================================================================================================================

void SpritePipeline::Reload()
{
    CreatePipeline();
}

//======================================================================================================================

void SpritePipeline::CreateDescriptorLayout()
{
    std::vector<VkDescriptorSetLayoutBinding> bindings{};

    // Sampler
    VkDescriptorSetLayoutBinding const combinedImageSampler{
        .binding = static_cast<uint32_t>(bindings.size()),
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
    };
    bindings.emplace_back(combinedImageSampler);

    _descriptorLayout = RB::CreateDescriptorSetLayout(
        LogicalDevice::Instance->GetVkDevice(),
        static_cast<uint8_t>(bindings.size()), bindings.data()
    );
}

//======================================================================================================================

void SpritePipeline::CreatePipeline()
{
    // Vertex shader
    {
        bool success = Importer::CompileShaderToSPV(
            Path::Instance()->Get("shaders/sprite_pipeline/SpritePipeline.vert.hlsl"),
            Path::Instance()->Get("shaders/sprite_pipeline/SpritePipeline.vert.spv"),
            "vert"
        );
        MFA_ASSERT(success == true);
    }
    auto cpuVertexShader = Importer::ShaderFromSPV(
        Path::Instance()->Get("shaders/sprite_pipeline/SpritePipeline.vert.spv"),
        VK_SHADER_STAGE_VERTEX_BIT,
        "main"
    );
    auto gpuVertexShader = RB::CreateShader(LogicalDevice::Instance->GetVkDevice(), cpuVertexShader);

    // Fragment shader
    {
        bool success = Importer::CompileShaderToSPV(
            Path::Instance()->Get("shaders/sprite_pipeline/SpritePipeline.frag.hlsl"),
            Path::Instance()->Get("shaders/sprite_pipeline/SpritePipeline.frag.spv"),
            "frag"
        );
        MFA_ASSERT(success == true);
    }
    auto cpuFragmentShader = Importer::ShaderFromSPV(
        Path::Instance()->Get("shaders/sprite_pipeline/SpritePipeline.frag.spv"),
        VK_SHADER_STAGE_FRAGMENT_BIT,
        "main"
    );
    auto gpuFragmentShader = RB::CreateShader(LogicalDevice::Instance->GetVkDevice(), cpuFragmentShader);

    std::vector<RT::GpuShader const *> shaders{gpuVertexShader.get(), gpuFragmentShader.get()};

	std::vector<VkVertexInputBindingDescription> const bindingDescriptions{
        VkVertexInputBindingDescription{
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        }
	};

    std::vector<VkVertexInputAttributeDescription> inputAttributeDescriptions{};
    // TODO: Automate this: Check how donut does this
    // Position
    inputAttributeDescriptions.emplace_back(MFA_VERTEX_INPUT_ATTRIBUTE(
        static_cast<uint32_t>(inputAttributeDescriptions.size()),
        0,
        Vertex,
        position
    ));
    // UV
    inputAttributeDescriptions.emplace_back(MFA_VERTEX_INPUT_ATTRIBUTE(
        static_cast<uint32_t>(inputAttributeDescriptions.size()),
        0,
        Vertex,
        uv
    ));

    RB::CreateGraphicPipelineOptions pipelineOptions{};
    pipelineOptions.useStaticViewportAndScissor = false;
    pipelineOptions.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipelineOptions.rasterizationSamples = LogicalDevice::Instance->GetMaxSampleCount();
    pipelineOptions.cullMode = VK_CULL_MODE_NONE;
    pipelineOptions.polygonMode = VK_POLYGON_MODE_FILL;
    pipelineOptions.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    pipelineOptions.depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    pipelineOptions.depthStencil.depthTestEnable = VK_TRUE;
    pipelineOptions.depthStencil.depthWriteEnable = VK_TRUE;
    pipelineOptions.depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    pipelineOptions.depthStencil.depthBoundsTestEnable = VK_FALSE;
    pipelineOptions.depthStencil.stencilTestEnable = VK_FALSE;
    pipelineOptions.colorBlendAttachments.blendEnable = VK_TRUE;
    pipelineOptions.colorBlendAttachments.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    pipelineOptions.colorBlendAttachments.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    pipelineOptions.colorBlendAttachments.colorBlendOp = VK_BLEND_OP_ADD;
    pipelineOptions.colorBlendAttachments.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    pipelineOptions.colorBlendAttachments.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    pipelineOptions.colorBlendAttachments.alphaBlendOp = VK_BLEND_OP_ADD;
    pipelineOptions.colorBlendAttachments.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    std::vector<VkPushConstantRange> pushConstantRanges{};
    {
        pushConstantRanges.emplace_back();
        auto &pushConstant = pushConstantRanges.back();
        pushConstant.size = sizeof(PushConstants);
        pushConstant.offset = 0;
        pushConstant.stageFlags = _pushConstantsStageFlags;        // This should be read from shader as reflection
    }

    // pipeline layout
    std::vector<VkDescriptorSetLayout> setLayout{_descriptorLayout->descriptorSetLayout};

    const auto pipelineLayout = RB::CreatePipelineLayout(
        LogicalDevice::Instance->GetVkDevice(),
        setLayout.size(),
        setLayout.data(),
        pushConstantRanges.size(),
        pushConstantRanges.data()
    );

    auto surfaceCapabilities = LogicalDevice::Instance->GetSurfaceCapabilities();

    _pipeline = RB::CreateGraphicPipeline(
        LogicalDevice::Instance->GetVkDevice(),
        static_cast<uint8_t>(shaders.size()),
        shaders.data(),
        bindingDescriptions.size(),
        bindingDescriptions.data(),
        static_cast<uint8_t>(inputAttributeDescriptions.size()),
        inputAttributeDescriptions.data(),
        surfaceCapabilities.currentExtent,
        _displayRenderPass->GetVkRenderPass(),
        pipelineLayout,
        pipelineOptions
    );
}

//======================================================================================================================
