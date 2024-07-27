if(BUILD_TESTING)
    enable_testing()

    add_executable(thread_pool_tests
        ./thread_pool_test.cpp
    )

    target_link_libraries(thread_pool_tests
        PRIVATE
            GTest::gtest_main
            thread_pool
    )

    include(GoogleTest)
    gtest_discover_tests(thread_pool_tests)
endif()
