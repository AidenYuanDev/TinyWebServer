# 添加静态库
add_library(router STATIC)

target_sources(router
    PRIVATE
        src/router.cpp
    PUBLIC
        include/router.h
)

target_include_directories(router
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${YAML_CPP_INCLUDE_DIRS}
)

target_link_libraries(router
    PRIVATE
    http_response
    http_request
    route
    http_types
)

# 测试
# add_subdirectory(test)
