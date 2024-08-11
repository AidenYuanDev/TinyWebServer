# 添加静态库
add_library(http_request STATIC)

target_sources(http_request
    PRIVATE
        src/http_request.cpp
    PUBLIC
        include/http_request.h
)

target_include_directories(http_request
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(http_request
    PRIVATE
    http_types
)

# 测试
# add_subdirectory(test)
