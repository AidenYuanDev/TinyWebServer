cmake_minimum_required(VERSION 3.14)
project(Server VERSION 1.0.0 LANGUAGES CXX)

# 包含自定义 CMake 模块
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# 包含项目设置和依赖查找
include(ProjectSettings)
include(FindDependencies)

# 添加子目录
add_subdirectory(src)
add_subdirectory(modules)
# add_subdirectory(test)

# 安装规则
install(DIRECTORY include/ DESTINATION include)


