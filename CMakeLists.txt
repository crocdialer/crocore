cmake_minimum_required(VERSION 3.3)

## define our project
project(crocore)

option(BUILD_SHARED_LIBS "Build Shared Libraries" ON)
option(BUILD_TESTS "Build Tests" ON)

## request C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -Wall -Wformat=0")

set(LIB_TYPE SHARED)
if (NOT BUILD_SHARED_LIBS)
    set(LIB_TYPE STATIC)
    set(Boost_USE_STATIC_LIBS ON)
endif (NOT BUILD_SHARED_LIBS)

## cmake modules
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules ${CMAKE_MODULE_PATH})
include(add_test_helper)

##### BOOST
set(BOOST_COMPONENTS system filesystem)
if (BUILD_TESTS)
    ENABLE_TESTING()
    set(BOOST_COMPONENTS ${BOOST_COMPONENTS} unit_test_framework)
    if (BUILD_SHARED_LIBS)
        add_definitions(-DBOOST_TEST_DYN_LINK)
    endif (BUILD_SHARED_LIBS)
endif (BUILD_TESTS)
find_package(Boost REQUIRED COMPONENTS ${BOOST_COMPONENTS})
include_directories(${Boost_INCLUDE_DIRS})
set(LIBS ${LIBS} ${Boost_LIBRARIES})
#####

##### CURL
find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIR})
set(LIBS ${LIBS} ${CURL_LIBRARY})
#####

include_directories(${PROJECT_SOURCE_DIR}/include)

add_subdirectory("src")

# Expose public includes (including Boost transitively) to other
# subprojects through cache variable.
set(${PROJECT_NAME}_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include
        ${Boost_INCLUDE_DIRS}
        CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)

set(${PROJECT_NAME}_LIBRARIES ${PROJECT_NAME}
        CACHE INTERNAL "${PROJECT_NAME}: Libraries" FORCE)
