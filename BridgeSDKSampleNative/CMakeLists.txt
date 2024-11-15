cmake_minimum_required(VERSION 3.1)

project("BridgeSDKSampleNative")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)

include_directories(${OPENGL_INCLUDE_DIRS})

set(GLFW_BUILD_DOCS OFF CACHE BOOL "GLFW lib only")
set(GLFW_INSTALL OFF CACHE BOOL "GLFW lib only")
set(GLAD_GL "")

if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

if (LINUX)
    # mlc: for the moment we only support X11
    set(GLFW_BUILD_X11 ON)
    set(GLFW_BUILD_WAYLAND OFF)
endif()

add_definitions(-DGLM_ENABLE_EXPERIMENTAL)
add_definitions(-D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)

add_subdirectory(glfw)

option(GLFW-CMAKE-STARTER-USE-GLFW-GLAD "Use GLAD from GLFW" ON)

include_directories(SYSTEM "${PROJECT_SOURCE_DIR}")
include_directories(SYSTEM "../BridgeRuntime")
if(GLFW-CMAKE-STARTER-USE-GLFW-GLAD)
    include_directories("${GLFW_SOURCE_DIR}/deps")
    set(GLAD_GL "${GLFW_SOURCE_DIR}/deps/glad/gl.h")
endif()

if(MSVC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ENTRY:mainCRTStartup")
endif()

set(GLFW-CMAKE-STARTER-SRC main.cpp)

add_executable("BridgeSDKSampleNative" WIN32 ${GLFW-CMAKE-STARTER-SRC} ${GLAD_GL})
target_link_libraries("BridgeSDKSampleNative" ${OPENGL_LIBRARIES} glfw)

if(MSVC)
    if(${CMAKE_VERSION} VERSION_LESS "3.6.0")
        message("\n\t[ WARNING ]\n\n\tCMake version lower than 3.6.\n\n\t - Please update CMake and rerun; OR\n\t - Manually set 'GLFW-CMake-starter' as StartUp Project in Visual Studio.\n")
    else()
        set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT "BridgeSDKSampleNative")
    endif()
endif()

# Copy /Assets/ folder to the output directory
add_custom_command(TARGET "BridgeSDKSampleNative" POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/Assets" $<TARGET_FILE_DIR:BridgeSDKSampleNative>/Assets
)
