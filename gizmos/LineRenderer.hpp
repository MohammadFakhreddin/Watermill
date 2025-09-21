#pragma once

#include "BedrockMath.hpp"
#include "LinePipeline.hpp"

namespace MFA
{

    class LineRenderer
    {
    public:

        explicit LineRenderer(std::shared_ptr<LinePipeline> linePipeline);
        // Note: This code is not optimized and is for debug purposes only
        void Draw(
            RT::CommandRecordState& recordState,
            glm::vec3 const& from,
            glm::vec3 const& to,
            glm::vec4 const& color = { 0.0f, 1.0f, 0.0f, 1.0f }
    );

    private:

        void PrepareBuffers(RT::CommandRecordState& recordState);

        std::shared_ptr<LinePipeline> _linePipeline{};
        std::shared_ptr<RT::BufferAndMemory> _vertexBuffer{};
        std::shared_ptr<RT::BufferAndMemory> _indexBuffer{};
        glm::vec3 _startPos{ -0.5f, 0.0f, 0.0f };
        glm::vec3 _endPos{ 0.5f, 0.0f, 0.0f };
        glm::vec3 _direction{};
        glm::vec3 _center{};
        float _length{};
        int _indexCount{};
    };

}