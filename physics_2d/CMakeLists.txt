
set(LIBRARY_SOURCES)

list(
    APPEND LIBRARY_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/Physics2D.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Physics2D.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/AABB2D.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/AABB2D.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/PhysicsBody.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/PhysicsBody.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Collider2d.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Collider2d.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/AABB_Collider.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/AABB_Collider.cpp"
)

set(LIBRARY_NAME "Physics2D")
add_library(${LIBRARY_NAME} ${LIBRARY_SOURCES})
target_link_libraries(${LIBRARY_NAME} glm)
target_link_libraries(${LIBRARY_NAME} Gizmos)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/")
