########################################

set(EXECUTABLE "TimeShift")

list(
    APPEND EXECUTABLE_RESOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/TimeShiftMain.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TimeShiftApp.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TimeShiftApp.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/IScene.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/MenuScene.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/MenuScene.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/GameScene.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/GameScene.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ScoreboardScene.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ScoreboardScene.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/GenerateGame.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/SpritePipeline.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/SpritePipeline.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/SpriteRenderer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/SpriteRenderer.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ResourceManager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ResourceManager.hpp"
)

### Linking libraries ####################################

add_executable(${EXECUTABLE} ${EXECUTABLE_RESOURCES})

target_link_libraries(${EXECUTABLE} glm)
target_link_libraries(${EXECUTABLE} Vulkan::Vulkan)
target_link_libraries(${EXECUTABLE} SDL2-static)
target_link_libraries(${EXECUTABLE} LibConfig)

target_link_libraries(${EXECUTABLE} Imgui)
target_link_libraries(${EXECUTABLE} Implot)
target_link_libraries(${EXECUTABLE} Bedrock)
target_link_libraries(${EXECUTABLE} AssetSystem)
target_link_libraries(${EXECUTABLE} EntitySystem)
target_link_libraries(${EXECUTABLE} Importer)
target_link_libraries(${EXECUTABLE} TimeSystem)
target_link_libraries(${EXECUTABLE} JobSystem)
target_link_libraries(${EXECUTABLE} RenderSystem)
target_link_libraries(${EXECUTABLE} Shared)
target_link_libraries(${EXECUTABLE} Webview)

########################################