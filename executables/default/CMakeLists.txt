########################################

set(EXECUTABLE "Default")

list(
    APPEND EXECUTABLE_RESOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/DefaultMain.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/EditorApp.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/EditorApp.hpp"
)

add_executable(${EXECUTABLE} ${EXECUTABLE_RESOURCES})

########################################