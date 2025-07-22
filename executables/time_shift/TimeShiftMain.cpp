#include "BedrockPlatforms.hpp"
#include "BedrockLog.hpp"
#include "BedrockPath.hpp"
#include "LevelParser.hpp"
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
#ifdef MFA_DEBUG
    MFA_LOG_INFO("Running in debug mode");
#else
    MFA_LOG_INFO("Running in release mode");
#endif


    MFA_ASSERT(device->IsValid() == true);
    {
        auto path = MFA::Path::Instance(true);

        auto jobSystem = MFA::JobSystem::Instantiate();
        auto resourceManager = ResourceManager::Instantiate();

        auto app = std::make_shared<TimeShiftApp>();
        app->Run();

        // We have to make sure all thread all complete
        ResourceManager::Destroy();
        JobSystem::Destroy();
    }

    MFA_LOG_INFO("End of a legend");

    return 0;
}