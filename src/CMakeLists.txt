FILE(GLOB FOLDER_SOURCES *.c*)
FILE(GLOB FOLDER_HEADERS *.h*)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
SET(LIB_NAME ${PROJECT_NAME})

add_library(${LIB_NAME} ${LIB_TYPE} ${FOLDER_SOURCES} ${FOLDER_HEADERS} ../include/crocore/gaussian.hpp)
TARGET_LINK_LIBRARIES(${LIB_NAME} ${LIBS})

SET(LIBS ${LIBS} ${LIB_NAME})
link_directories(${CMAKE_CURRENT_BINARY_DIR})

SOURCE_GROUP("Headers" FILES ${FOLDER_HEADERS})
SOURCE_GROUP("Sources" FILES ${FOLDER_SOURCES})

#Boost Unit-Tests
addTestMacro()

install (TARGETS ${LIB_NAME} DESTINATION lib)    
install (FILES ${FOLDER_HEADERS} DESTINATION "include/${LIB_NAME}")
