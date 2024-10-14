include(FindGTest)
find_package(GTest REQUIRED)

add_executable(test_threadpool onethreadpool.cpp threadpool.cpp)
target_link_libraries(test_threadpool PRIVATE ${PROJECT_NAME} GTest::gtest GTest::gtest_main GTest::gmock)
set_target_properties(test_threadpool PROPERTIES CXX_STANDARD 20)

if(ENABLE_MAINTAINER_MODE)
    if(CMAKE_COMPILER_IS_CLANG)
        target_compile_options(test_threadpool PRIVATE -Wno-weak-vtables -Wno-global-constructors -Wno-padded)
    endif()
endif()

if(NOT CMAKE_CROSSCOMPILING)
    include(GoogleTest)
    gtest_discover_tests(test_threadpool)
endif()

if(ENABLE_COVERAGE)
    if(UNIX)
        add_custom_command(
            TARGET test_threadpool
            POST_BUILD
            COMMAND find . -type f -name \*.gcda -delete
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )
    endif()

    add_custom_target(
        coverage
        COMMAND ctest -C Coverage -T test --output-on-failure
        COMMAND rm -rf "${PROJECT_BINARY_DIR}/coverage"
        COMMAND mkdir -p "${PROJECT_BINARY_DIR}/coverage"
        COMMAND
            gcovr -f "${PROJECT_SOURCE_DIR}/src/" -r "${PROJECT_SOURCE_DIR}"
                --html-details -o "${PROJECT_BINARY_DIR}/coverage/index.html"
                --exclude-noncode-lines --exclude-throw-branches --exclude-unreachable-branches --decisions
                --gcov-executable="${GCOV_TOOL}"
                --print-summary
                --txt "${PROJECT_BINARY_DIR}/coverage/coverage.txt"
                --lcov "${PROJECT_BINARY_DIR}/coverage/coverage.lcov"
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    )
endif()
