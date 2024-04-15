file(GLOB SERVER_HEADERS "*.h")
file(GLOB SERVER_SOURCES "*.cpp")

add_executable(client ${SERVER_HEADERS} ${SERVER_SOURCES})

target_link_libraries(client PUBLIC ${Boost_LIBRARIES})
