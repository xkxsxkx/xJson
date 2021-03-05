cmake_minimum_required(VERSION 3.0.0)
project(xjson VERSION 0.1.0)

set(INCLUDE_ALL_DIR
    ${CMAKE_SOURCE_DIR}/include
)

set(LIB_ALL_DIR
    ${CMAKE_SOURCE_DIR}/lib
)

include_directories(${INCLUDE_ALL_DIR})
link_directories(${LIB_ALL_DIR})

aux_source_directory(${CMAKE_SOURCE_DIR}/src DIR_SRCS)

set(ALL_SRCS
    ${DIR_SRCS}
)

add_library(xjson ${ALL_SRCS})

include(CTest)
enable_testing()

add_subdirectory(tests)

file(GLOB_RECURSE TEST_FILES ${CMAKE_SOURCE_DIR}/tests/xtest_*.cpp)
foreach(FILE_PATH ${TEST_FILES})
    get_filename_component(FILE ${FILE_PATH} NAME)
    get_filename_component(FILE_NAME ${FILE_PATH} NAME_WE)
    MESSAGE(${FILE_NAME})
    MESSAGE(${FILE})
    add_test(
        NAME ${FILE_NAME}
        COMMAND ${FILE_NAME}
        # WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/3rdparty/opencv/bin
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
    )
    install(TARGETS ${FILE_NAME};
        RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/bin
    )
endforeach(FILE_PATH)

install(TARGETS xjson
    ARCHIVE DESTINATION ${CMAKE_SOURCE_DIR}/lib
    RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/bin
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
