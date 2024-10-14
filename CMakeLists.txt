cmake_minimum_required(VERSION 3.23)

set(EXPORT_COMPILE_COMMANDS ON)

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(BUILD_TESTS "Build tests" ON)
option(ENABLE_COVERAGE "Enable coverage" OFF)
option(ENABLE_MAINTAINER_MODE "Enable maintainer mode" OFF)

project(
    wwa-simple-threadpool
    VERSION 1.0.0
    LANGUAGES CXX
)

set(CMAKE_VERBOSE_MAKEFILE ON)

string(REGEX MATCH "Clang" CMAKE_COMPILER_IS_CLANG "${CMAKE_CXX_COMPILER_ID}")
string(REGEX MATCH "GNU" CMAKE_COMPILER_IS_GNU "${CMAKE_CXX_COMPILER_ID}")

if(BUILD_TESTS)
    include(FindGTest)
    find_package(GTest CONFIG COMPONENTS gtest gmock)
    if(NOT TARGET GTest::gtest OR NOT TARGET GTest::gmock)
        message(WARNING "GTest not found, tests will not be built")
        set(BUILD_TESTS OFF)
    endif()
endif()

if(NOT BUILD_TESTS)
    set(ENABLE_COVERAGE OFF)
endif()

macro(disable_coverage reason)
    message(WARNING "${reason}, coverage will not be enabled")
    set(ENABLE_COVERAGE OFF)
    if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
        set(CMAKE_BUILD_TYPE "Debug")
    endif()
endmacro(disable_coverage)

if(ENABLE_COVERAGE OR CMAKE_BUILD_TYPE STREQUAL "Coverage")
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
                set(CMAKE_BUILD_TYPE "Coverage")
            else()
                disable_coverage("${GCOV_TOOL_NAME} not found")
            endif()
        else()
            disable_coverage("gcovr not found")
        endif()
    else()
        disable_coverage("coverage is only supported with clang or GCC")
    endif()
endif()

if(CMAKE_COMPILER_IS_GNU)
    set(CMAKE_CXX_FLAGS_COVERAGE "-Og -g3 --coverage -fprofile-abs-path")
elseif(CMAKE_COMPILER_IS_CLANG)
    set(CMAKE_CXX_FLAGS_COVERAGE "-Og -g3 --coverage")
endif()

if(CMAKE_COMPILER_IS_GNU OR CMAKE_COMPILER_IS_CLANG)
    set(CMAKE_EXE_LINKER_FLAGS_COVERAGE "--coverage")
    set(CMAKE_MODULE_LINKER_FLAGS_COVERAGE "--coverage")
    set(CMAKE_SHARED_LINKER_FLAGS_COVERAGE "--coverage")
endif()

if(ENABLE_MAINTAINER_MODE)
    if(CMAKE_COMPILER_IS_CLANG)
        add_compile_options(-Weverything -Werror -Wno-c++98-compat -Wno-c++98-compat-pedantic -pedantic)
    elseif(CMAKE_COMPILER_IS_GNU)
        add_compile_options(-Wall -Wextra -Werror -pedantic)
    endif()
endif()

find_package(Threads REQUIRED)

add_library(${PROJECT_NAME})
target_sources(
    ${PROJECT_NAME}
    PUBLIC
        FILE_SET HEADERS
        TYPE HEADERS
        BASE_DIRS src
        FILES
            src/export.h
            src/threadpool.h
    PRIVATE
        src/threadpool.cpp
        src/threadpool_p.cpp
)
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
target_include_directories(
    ${PROJECT_NAME}
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
)
set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
        POSITION_INDEPENDENT_CODE ON
)
if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PUBLIC WWA_SIMPLE_THREADPOOL_STATIC_DEFINE)
endif()

if(BUILD_TESTS)
    include(CTest)
    enable_testing()
    add_subdirectory(test)
endif()

find_program(CLANG_FORMAT NAMES clang-format)
find_program(CLANG_TIDY NAMES clang-tidy)

if(CLANG_FORMAT)
    add_custom_target(
        format
        COMMAND ${CLANG_FORMAT} --Wno-error=unknown -i -style=file ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h ${CMAKE_CURRENT_SOURCE_DIR}/test/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/test/*.h
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()

if(CLANG_TIDY)
    add_custom_target(
        tidy
        COMMAND ${CLANG_TIDY} -p ${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/test/*.cpp
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)
install(
    TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}-target
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    FILE_SET HEADERS DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}"
)

install(
    EXPORT ${PROJECT_NAME}-target
    FILE ${PROJECT_NAME}-target.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PROJECT_NAME}-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
    COPYONLY
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

configure_file("${CMAKE_SOURCE_DIR}/cmake/pkg-config.pc.in" "${PROJECT_NAME}.pc" @ONLY)
install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
)
