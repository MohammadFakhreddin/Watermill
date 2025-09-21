#pragma once

#include "BedrockMath.hpp"
#include "PointPipeline.hpp"

namespace MFA
{
    class PointRenderer
    {
    public:

        explicit PointRenderer(std::shared_ptr<PointPipeline> pointPipeline);
        // Note: This function is not optimized and is used for debug purposes only!
        void Draw(
            RT::CommandRecordState& recordState,
            glm::vec3 const& position,
            glm::vec4 const& color = { 1.0f, 0.0f, 0.0f, 1.0f },
            float pointSize = 10.0f
        );

    private:

        void PrepareBuffers(RT::CommandRecordState& recordState);

        std::shared_ptr<PointPipeline> _pointPipeline{};
        std::shared_ptr<RT::BufferAndMemory> _vertexBuffer{};
        std::shared_ptr<RT::BufferAndMemory> _indexBuffer{};
        int _indexCount{};
    };
}
