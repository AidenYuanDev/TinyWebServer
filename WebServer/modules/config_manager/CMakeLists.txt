
# 添加静态库
add_library(config_manager STATIC)

target_sources(config_manager
    PRIVATE
        src/config_manager.cpp
    PUBLIC
        include/config_manager.h 
)

target_include_directories(config_manager
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${YAML_CPP_INCLUDE_DIRS}
)

target_link_libraries(config_manager
    PRIVATE
    thread_pool
    message_queue 
    http_parser
    http_response
    logger
    ${YAML_CPP_LIBRARIES} 
)

# 测试
# add_subdirectory(test)
