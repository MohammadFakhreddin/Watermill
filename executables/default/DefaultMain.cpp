#include "BedrockLog.hpp"
#include "EditorApp.hpp"
#include "LogicalDevice.hpp"

using namespace MFA;

int main()
{
    LogicalDevice::InitParams params{.windowWidth = 1920,
                                     .windowHeight = 1080,
                                     .resizable = true,
                                     .fullScreen = false,
                                     .applicationName = "InverseKinematics"};

    auto device = LogicalDevice::Instantiate(params);
    assert(device->IsValid() == true);
    {
        EditorApp app{};
        app.Run();
    }

    return 0;
}