cmake_minimum_required(VERSION 3.16)

project(SDL2-Skeleton)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Point this at your local SDL2 install root.
# or set an environment variable SDL2_ROOT before configuring.
if(NOT SDL2_ROOT AND DEFINED ENV{SDL2_ROOT})
    set(SDL2_ROOT $ENV{SDL2_ROOT})
endif()

if(SDL2_ROOT)
    set(CMAKE_PREFIX_PATH
        "${SDL2_ROOT}/x86_64-w64-mingw32"
        "${SDL2_ROOT}_image/x86_64-w64-mingw32"
        "${SDL2_ROOT}_ttf/x86_64-w64-mingw32"
        "${SDL2_ROOT}_mixer/x86_64-w64-mingw32"
    )
endif()

find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_ttf REQUIRED)
find_package(SDL2_mixer REQUIRED)

set(SOURCE_FILES
    src/main.cpp
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        SDL2::SDL2main
        $<$<CONFIG:Release>:SDL2::SDL2-static>
        $<$<CONFIG:Release>:SDL2_image::SDL2_image-static>
        $<$<CONFIG:Release>:SDL2_ttf::SDL2_ttf-static>
        $<$<CONFIG:Release>:SDL2_mixer::SDL2_mixer-static>
        $<$<CONFIG:Debug>:SDL2::SDL2>
        $<$<CONFIG:Debug>:SDL2_image::SDL2_image>
        $<$<CONFIG:Debug>:SDL2_ttf::SDL2_ttf>
        $<$<CONFIG:Debug>:SDL2_mixer::SDL2_mixer>
)

if(MINGW)
    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:-static-libgcc>
        $<$<CONFIG:Release>:-static-libstdc++>
        $<$<CONFIG:Release>:-static>
    )
endif()

if(SDL2_ROOT)
    target_include_directories(${PROJECT_NAME}
        PRIVATE
            "${SDL2_ROOT}/x86_64-w64-mingw32/include"
            "${SDL2_ROOT}_image/x86_64-w64-mingw32/include"
            "${SDL2_ROOT}_ttf/x86_64-w64-mingw32/include"
            "${SDL2_ROOT}_mixer/x86_64-w64-mingw32/include"
    )
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:/W4>
        $<$<CONFIG:Release>:/W4 /O2>
    )
else()
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:-Wall -Wextra>
        $<$<CONFIG:Release>:-Wall -Wextra -O3>
    )
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}"
)

# Assets are expected inside the project source tree by default (portable).
# Override with -DASSETS_SOURCE_DIR=... if you keep them elsewhere.
if(NOT ASSETS_SOURCE_DIR)
    set(ASSETS_SOURCE_DIR "${CMAKE_SOURCE_DIR}/assets")
endif()

if(WIN32 AND SDL2_ROOT AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL2_ROOT}/x86_64-w64-mingw32/bin/SDL2.dll"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL2_ROOT}_image/x86_64-w64-mingw32/bin/SDL2_image.dll"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL2_ROOT}_ttf/x86_64-w64-mingw32/bin/SDL2_ttf.dll"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL2_ROOT}_mixer/x86_64-w64-mingw32/bin/SDL2_mixer.dll"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
endif()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${ASSETS_SOURCE_DIR}"
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
)