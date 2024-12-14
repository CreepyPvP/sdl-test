cmake_minimum_required(VERSION 3.25)

project(sdltest)

set(CMAKE_BUILD_TYPE Debug)
set(ROOT_DIR ${PROJECT_SOURCE_DIR})

add_subdirectory(external/sdl EXCLUDE_FROM_ALL)


file(GLOB_RECURSE SOURCE_FILES
    ${CMAKE_SOURCE_DIR}/code/main.cpp
)

add_executable(${PROJECT_NAME} 
    ${SOURCE_FILES}
)

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:SDL3::SDL3>" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    VERBATIM
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_SOURCE_DIR}/external/sdl/include
    ${CMAKE_SOURCE_DIR}/external/sdl/src
    ${CMAKE_SOURCE_DIR}/external/sdl/include/build_config
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    SDL3::SDL3
)
