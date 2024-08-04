# 添加静态库
add_library(route_manager STATIC)

target_sources(route_manager
    PRIVATE
        src/route_manager.cpp
    PUBLIC
        include/route_manager.h
)

target_include_directories(route_manager
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(route_manager
    PRIVATE
    http_parser
    http_response
    logger
    config_manager
    yaml-cpp
)

# 测试
# add_subdirectory(test)
