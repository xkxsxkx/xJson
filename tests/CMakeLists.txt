CMAKE_MINIMUM_REQUIRED(VERSION 3.4)

project(Tests)

include_directories(${INCLUDE_ALL_DIR})

link_directories(
    ../lib
)

file(GLOB_RECURSE TEST_FILES "xtest_*.cpp")
foreach(FILE_PATH ${TEST_FILES})
    MESSAGE(${FILE_PATH})
    get_filename_component(FILE ${FILE_PATH} NAME)
    get_filename_component(FILE_NAME ${FILE_PATH} NAME_WE)
    MESSAGE(${FILE_NAME})
    MESSAGE(${FILE})
    add_executable(${FILE_NAME} ${FILE})
    target_link_libraries(${FILE_NAME} xjson)
endforeach(FILE_PATH)