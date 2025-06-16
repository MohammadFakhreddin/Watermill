#include "BedrockLog.hpp"
#include "BedrockPath.hpp"
#include "GenerateGame.h"
#include "JobSystem.hpp"
#include "LogicalDevice.hpp"
#include "ResourceManager.hpp"
#include "TimeShiftApp.hpp"

using namespace MFA;

int main()
{
    LogicalDevice::InitParams params{.windowWidth = 1920,
                                     .windowHeight = 1080,
                                     .resizable = true,
                                     .fullScreen = false,
                                     .applicationName = "Timeshift"};

    auto device = LogicalDevice::Instantiate(params);

    if (SDL_JoystickOpen(0) != nullptr) SDL_JoystickEventState(SDL_ENABLE);

    MFA_LOG_INFO("Beginning of a legend");

    MFA_ASSERT(device->IsValid() == true);
    {
        auto path = MFA::Path::Instance();
        auto jobSystem = MFA::JobSystem::Instance();
        auto resourceManager = ResourceManager::Instance();

        TimeShiftApp app{};
        app.Run();
    }

    MFA_LOG_INFO("End of a legend");

    return 0;
}