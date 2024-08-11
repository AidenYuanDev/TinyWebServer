# 添加静态库
add_library(http_parser STATIC)

target_sources(http_parser
    PRIVATE
        src/http_parser.cpp
    PUBLIC
        include/http_parser.h
)

target_include_directories(http_parser
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(http_parser
    PRIVATE
    http_types
    http_request
)

# 测试
# add_subdirectory(test)
