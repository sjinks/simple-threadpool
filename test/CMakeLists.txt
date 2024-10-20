include(FindGTest)
find_package(GTest REQUIRED)

add_executable(test_threadpool onethreadpool.cpp threadpool.cpp)
target_link_libraries(test_threadpool PRIVATE ${PROJECT_NAME} GTest::gmock_main)
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

set(ENABLE_COVERAGE OFF)
if("coverage" IN_LIST CMAKE_CONFIGURATION_TYPES_LOWER OR "coverage" STREQUAL CMAKE_BUILD_TYPE_LOWER)
    if(CMAKE_COMPILER_IS_GNU OR CMAKE_COMPILER_IS_CLANG)
        find_program(GCOVR gcovr)
        if (GCOVR)
            if(CMAKE_COMPILER_IS_GNU)
                find_program(GCOV gcov)
                set(GCOV_TOOL_NAME gcov)
                set(GCOV_TOOL gcov)
            elseif(CMAKE_COMPILER_IS_CLANG)
                find_program(GCOV llvm-cov)
                set(GCOV_TOOL_NAME llvm-cov)
                set(GCOV_TOOL llvm-cov gcov)
            endif()

            if(GCOV)
                set(ENABLE_COVERAGE ON)
            else()
                message(WARNING "${GCOV_TOOL_NAME} not found, coverage report will not be generated")
            endif()
        else()
            message(WARNING "gcovr not found, coverage report will not be generated")
        endif()
    endif()
endif()

if(ENABLE_COVERAGE)
    add_custom_target(
        coverage
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${PROJECT_BINARY_DIR}/coverage"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/coverage"
        COMMAND ${CMAKE_CTEST_COMMAND} -C $<CONFIG> -T test --output-on-failure
        COMMAND
            gcovr -f "${PROJECT_SOURCE_DIR}/src/" -r "${PROJECT_SOURCE_DIR}"
                --html-details -o "${PROJECT_BINARY_DIR}/coverage/index.html"
                --exclude-noncode-lines --exclude-throw-branches --exclude-unreachable-branches --decisions
                --gcov-executable="${GCOV_TOOL}"
                --print-summary
                --txt "${PROJECT_BINARY_DIR}/coverage/coverage.txt"
                --lcov "${PROJECT_BINARY_DIR}/coverage/coverage.lcov"
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
        USES_TERMINAL
    )
    add_dependencies(coverage test_threadpool)
endif()
