add_executable(${PROJECT_NAME} main.cpp)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    thread_pool
    client_context
    server 
    http_parser
    http_response
    logger
    config_manager
    yaml-cpp
    route_manager
)

# 复制 public 目录到构建目录
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/public $<TARGET_FILE_DIR:${PROJECT_NAME}>/public
)

# 复制配置文件到构建目录
configure_file(${CMAKE_SOURCE_DIR}/config/server_config.yaml
               ${CMAKE_BINARY_DIR}/bin/server_config.yaml
               COPYONLY)

