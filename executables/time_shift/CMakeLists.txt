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
)

add_executable(${EXECUTABLE} ${EXECUTABLE_RESOURCES})

########################################