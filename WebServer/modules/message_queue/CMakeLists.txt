# 添加静态库
add_library(message_queue STATIC)

target_sources(message_queue
    PRIVATE
        ./src/message_queue.cpp 
    PUBLIC
        ./include/message_queue.h
)

target_include_directories(message_queue
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${YAML_CPP_INCLUDE_DIRS} 
)

target_link_libraries(message_queue
    PRIVATE
    http_parser
    http_response
    http_request
    http_types
    logger
    config_manager
    ${YAML_CPP_LIBRARIES} 
)

# 测试
# add_subdirectory(test)
