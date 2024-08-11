# 添加静态库
add_library(http_types STATIC)

target_sources(http_types
    PRIVATE
        src/http_types.cpp
    PUBLIC
        include/http_types.h
)

target_include_directories(http_types
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(http_types
    PRIVATE
    http_request
    http_response
)

# 测试
# add_subdirectory(test)
