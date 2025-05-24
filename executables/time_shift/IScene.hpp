#pragma once

#include "RenderTypes.hpp"

class IScene
{
    void Update(float deltaTime) = 0;

    void Render(MFA::RT::CommandRecordState & recordState) = 0;

}
