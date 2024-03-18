
set(EXEC_NAME logger_test)
enable_testing()

add_executable(${EXEC_NAME}
    logger_test.cpp
)

target_link_libraries(${EXEC_NAME}
    GTest::gmock
    logger
)

include(GoogleTest)
gtest_discover_tests(${EXEC_NAME})
