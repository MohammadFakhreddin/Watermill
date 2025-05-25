#include "BedrockLog.hpp"
#include "GenerateGame.h"
#include "LogicalDevice.hpp"
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

    assert(device->IsValid() == true);
    {
        TimeShiftApp app{};
        app.Run();
    }
    MFA_LOG_INFO("Beginning of a legend");

    return 0;
}