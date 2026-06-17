cmake_minimum_required(VERSION 3.16)
project(IronCut VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags for maximum performance
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(OpenGL REQUIRED)

# Find FFmpeg components
pkg_check_modules(FFMPEG REQUIRED 
    libavcodec
    libavformat
    libavutil
    libswscale
    libswresample
)

# Download and build Dear ImGui with GLFW and OpenGL3 backend
include(FetchContent)

# GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.8
)
FetchContent_MakeAvailable(glfw)

# Dear ImGui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.90.1
)
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
endif()

# Set up ImGui sources
set(IMGUI_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${FFMPEG_INCLUDE_DIRS}
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${glfw_SOURCE_DIR}/include
    ${OPENGL_INCLUDE_DIR}
)

# Source files
set(SOURCES
    src/main.cpp
    src/clip.cpp
    src/timeline.cpp
    src/engine.cpp
    src/gui/application.cpp
    src/gui/widgets.cpp
    ${IMGUI_SOURCES}
)

# Create executable
add_executable(ironcut ${SOURCES})

# Link libraries
target_link_libraries(ironcut PRIVATE
    ${FFMPEG_LIBRARIES}
    glfw
    ${OPENGL_LIBRARIES}
    dl
)

# Add compiler definitions
target_compile_definitions(ironcut PRIVATE
    IMGUI_IMPL_OPENGL_LOADER_CUSTOM
)

# Installation
install(TARGETS ironcut DESTINATION bin)
