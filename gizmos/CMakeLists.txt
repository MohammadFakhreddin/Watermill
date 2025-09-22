set(LIBRARY_SOURCES)

list(
    APPEND LIBRARY_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/LinePipeline.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/LinePipeline.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/PointPipeline.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/PointPipeline.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/LineRenderer.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/LineRenderer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/PointRenderer.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/PointRenderer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Gizmos.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Gizmos.cpp"

)

set(LIBRARY_NAME "Gizmos")
add_library(${LIBRARY_NAME} ${LIBRARY_SOURCES})
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/")
target_link_libraries(${LIBRARY_NAME} RenderSystem)
