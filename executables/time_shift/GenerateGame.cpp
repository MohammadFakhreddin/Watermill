#include "GenerateGame.h"

#include <fstream>
#include <iostream>
#include <json.hpp>

#include "BedrockLog.hpp"

using namespace nlohmann;

GenerateGame::GenerateGame(std::filesystem::path json_path)
{
    std::ifstream file(json_path);
    json data = json::parse(file);
    if (data.is_discarded())
    {
        MFA_LOG_INFO("JSON file is invalid");
        return;
    }

    std::cout << data.dump(4) << std::endl;
}