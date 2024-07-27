if(BUILD_TESTING)
    enable_testing()

    add_executable(unit_tests
    )

    target_link_libraries(unit_tests
        PRIVATE
            GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(unit_tests)
endif()
