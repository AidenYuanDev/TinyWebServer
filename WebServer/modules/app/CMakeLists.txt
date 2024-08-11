# 添加静态库
add_library(app STATIC)

target_sources(app
    PRIVATE
        src/app.cpp
    PUBLIC
        include/app.h
)

target_include_directories(app
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${YAML_CPP_INCLUDE_DIRS}
)

target_link_libraries(app
    PRIVATE
    server 
    thread_pool
    message_queue 
    http_parser
    http_response
    http_types
    http_request
    route
    router
    static_file_controller
    logger
    config_manager
    ${YAML_CPP_LIBRARIES}
)

# 测试
# add_subdirectory(test)
