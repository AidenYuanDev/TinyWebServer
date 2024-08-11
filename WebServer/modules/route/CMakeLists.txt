# 添加静态库
add_library(route STATIC)

target_sources(route
    PRIVATE
        src/route.cpp
    PUBLIC
        include/route.h
)

target_include_directories(route
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${YAML_CPP_INCLUDE_DIRS}
)

target_link_libraries(route
    PRIVATE
    http_parser
    http_response
    http_types
    http_request
)

# 测试
# add_subdirectory(test)
