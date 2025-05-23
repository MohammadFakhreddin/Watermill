########################################

set(EXECUTABLE "Default")

list(
    APPEND EXECUTABLE_RESOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/DefaultMain.cpp"
)

add_executable(${EXECUTABLE} ${EXECUTABLE_RESOURCES})

########################################