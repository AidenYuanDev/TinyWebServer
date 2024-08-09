# include(FetchContent)
#
# # 添加 Google Test
# FetchContent_Declare(
#   googletest
#   GIT_REPOSITORY https://github.com/google/googletest.git
#   GIT_TAG       main 
# )
# # For Windows: Prevent overriding the parent project's compiler/linker settings
# set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
# # gmock
# set(BUILD_GMOCK OFF CACHE BOOL "" FORCE) 
# # gtest
# set(BUILD_GTEST ON CACHE BOOL "" FORCE)
# FetchContent_MakeAvailable(googletest)

# include(FetchContent)
#
# FetchContent_Declare(
#   yaml-cpp
#   GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
#   GIT_TAG master # Can be a tag (yaml-cpp-x.x.x), a commit hash, or a branch name (master)
# )
# FetchContent_GetProperties(yaml-cpp)
#
# if(NOT yaml-cpp_POPULATED)
#   message(STATUS "Fetching yaml-cpp...")
#   FetchContent_Populate(yaml-cpp)
#   add_subdirectory(${yaml-cpp_SOURCE_DIR} ${yaml-cpp_BINARY_DIR})
# endif()

# 查找PkgConfig包
find_package(PkgConfig REQUIRED)

# 使用PkgConfig查找yaml-cpp
pkg_check_modules(YAML_CPP REQUIRED yaml-cpp)
