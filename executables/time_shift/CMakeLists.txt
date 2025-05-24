########################################

set(EXECUTABLE "TimeShift")

list(
        APPEND EXECUTABLE_RESOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/TimeShiftMain.cpp"
)

add_executable(${EXECUTABLE} ${EXECUTABLE_RESOURCES})

########################################