#include "BedrockLog.hpp"
#include "BedrockPath.hpp"
#include "GenerateGame.h"
#include "JobSystem.hpp"
#include "LogicalDevice.hpp"
#include "ResourceManager.hpp"
#include "TimeShiftApp.hpp"

#include "ImportTexture.hpp"

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
        auto path = MFA::Path::Instance(true);

        auto texture = Importer::LoadKtxMetadata(path->Get("textures/sky-0.ktx2").c_str());

        return 0;

        auto resourceManager = ResourceManager::Instance(true);
        auto jobSystem = MFA::JobSystem::Instance(true);

        auto app = std::make_shared<TimeShiftApp>();
        app->Run();

        // We have to make sure all thread all complete
        jobSystem.reset();
        resourceManager.reset();
    }

    MFA_LOG_INFO("End of a legend");

    return 0;
}