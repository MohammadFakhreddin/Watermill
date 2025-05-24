#include "BedrockLog.hpp"
#include "LogicalDevice.hpp"

using namespace MFA;

int main()
{
    LogicalDevice::InitParams params{.windowWidth = 1920,
                                     .windowHeight = 1080,
                                     .resizable = true,
                                     .fullScreen = false,
                                     .applicationName = "Timeshift"};

    auto device = LogicalDevice::Instantiate(params);
    assert(device->IsValid() == true);
//    {
//        EditorApp app{};
//        app.Run();
//    }
    MFA_LOG_INFO("Beginning of a legend");

    return 0;
}