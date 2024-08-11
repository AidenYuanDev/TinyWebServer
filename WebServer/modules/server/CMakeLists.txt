# 添加静态库
add_library(server STATIC)

target_sources(server
    PRIVATE
        src/server.cpp
    PUBLIC
        include/server.h
)

target_include_directories(server
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${YAML_CPP_INCLUDE_DIRS}
)

target_link_libraries(server
    PRIVATE
    thread_pool
    message_queue 
    http_parser
    http_types
    http_request
    http_response
    route
    router
    static_file_controller
    logger
    config_manager
    ${YAML_CPP_LIBRARIES}
)

# 测试
# add_subdirectory(test)
