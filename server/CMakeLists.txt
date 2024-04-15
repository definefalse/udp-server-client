cmake_minimum_required(VERSION ${CMAKE_VERSION})

project(server)

file(GLOB_RECURSE SERVER_HEADERS "*.h")
file(GLOB_RECURSE SERVER_SOURCES "*.cpp")
file(GLOB_RECURSE RESOURCE_FILE "*.json")

add_executable(server ${SERVER_HEADERS} ${SERVER_SOURCES} ${RESOURCE_FILE})

target_link_libraries(server PUBLIC ${Boost_LIBRARIES})

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/resources.json" "resources.json"
)
