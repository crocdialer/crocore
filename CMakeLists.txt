cmake_minimum_required(VERSION 3.12)

## define our project
project(crocore)

option(BUILD_SHARED_LIBS "Build Shared Libraries" OFF)
option(BUILD_TESTS "Build Tests" ON)

## request C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(LIB_NAME ${PROJECT_NAME})

if (BUILD_SHARED_LIBS)
    set(LIB_TYPE SHARED)
else()
    set(LIB_TYPE STATIC)
endif (BUILD_SHARED_LIBS)

## cmake modules
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules ${CMAKE_MODULE_PATH})

set(LIBS ${LIBS})

##### spdlog
add_subdirectory(submodules/spdlog EXCLUDE_FROM_ALL)
set(LIBS ${LIBS} spdlog)

include_directories(${PROJECT_SOURCE_DIR}/include)

# add library-target
add_subdirectory("src")

if(MSVC)
    target_compile_options(${LIB_NAME} PRIVATE /W4 /WX)
else()
    target_compile_options(${LIB_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

if (BUILD_TESTS)
    ##### gtest
    add_subdirectory(submodules/googletest EXCLUDE_FROM_ALL)

    enable_testing()
    add_subdirectory(tests)
endif (BUILD_TESTS)

# Expose public includes (including Boost transitively) to other
# subprojects through cache variable.
set(${PROJECT_NAME}_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include
        ${Boost_INCLUDE_DIRS}
        CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)

set(${PROJECT_NAME}_LIBRARIES ${PROJECT_NAME}
        CACHE INTERNAL "${PROJECT_NAME}: Libraries" FORCE)
