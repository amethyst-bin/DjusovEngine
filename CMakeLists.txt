cmake_minimum_required(VERSION 3.20)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)

project(DjusovEngine C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Optimization and warning flags
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers)
endif()

# Find System Dependencies
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glm REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(Threads REQUIRED)

# Build Luau Compiler and VM
set(LUAU_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LUAU_BUILD_CLI OFF CACHE BOOL "" FORCE)
set(LUAU_BUILD_WEB OFF CACHE BOOL "" FORCE)
set(LUAU_WERROR OFF CACHE BOOL "" FORCE)
add_subdirectory(src/vendor/luau EXCLUDE_FROM_ALL)

# ImGui vendored library
set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/vendor/imgui")
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# TextEditor vendored library
set(TEXTEDITOR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/vendor/TextEditor")
set(TEXTEDITOR_SOURCES
    ${TEXTEDITOR_DIR}/TextEditor.cpp
)

# ufbx vendored library
set(UFBX_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/vendor/ufbx")
set(UFBX_SOURCES
    ${UFBX_DIR}/ufbx.c
)

# miniaudio vendored library
set(MINIAUDIO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/vendor/miniaudio")
set(MINIAUDIO_SOURCES
    ${MINIAUDIO_DIR}/miniaudio.c
)

add_library(DjusovVendor STATIC
    ${IMGUI_SOURCES}
    ${TEXTEDITOR_SOURCES}
    ${UFBX_SOURCES}
    ${MINIAUDIO_SOURCES}
)

target_include_directories(DjusovVendor PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${TEXTEDITOR_DIR}
    ${UFBX_DIR}
    ${MINIAUDIO_DIR}
    ${GLFW_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIR}
)

target_link_libraries(DjusovVendor PUBLIC
    glfw
    GLEW::GLEW
    OpenGL::GL
    ${CMAKE_DL_LIBS}
    m
)

# Common engine include directories
set(ENGINE_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vendor/luau/VM/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vendor/luau/Compiler/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vendor/luau/Common/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vendor/luau/Ast/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vendor/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vendor/imgui/backends
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vendor/TextEditor
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vendor/ufbx
)

# Core engine library sources
file(GLOB_RECURSE ENGINE_CORE_SOURCES
    src/core/*.cpp
    src/render/*.cpp
    src/audio/*.cpp
    src/world/*.cpp
    src/player/*.cpp
    src/script/*.cpp
    src/network/*.cpp
    src/ui/*.cpp
    src/build/*.cpp
)

add_library(DjusovCore STATIC
    ${ENGINE_CORE_SOURCES}
)

target_include_directories(DjusovCore PUBLIC ${ENGINE_INCLUDES})
target_link_libraries(DjusovCore PUBLIC
    DjusovVendor
    Luau.Compiler
    Luau.VM
    glm::glm
    nlohmann_json::nlohmann_json
    Threads::Threads
)

# 1. Main Editor Executable: DjusovEngine
add_executable(DjusovEngine src/main.cpp)
target_include_directories(DjusovEngine PRIVATE ${ENGINE_INCLUDES})
target_link_libraries(DjusovEngine PRIVATE DjusovCore)

# 2. Standalone Game Runner Executable: djusov_runner
add_executable(djusov_runner src/runner_main.cpp)
target_include_directories(djusov_runner PRIVATE ${ENGINE_INCLUDES})
target_link_libraries(djusov_runner PRIVATE DjusovCore)

# 3. Game Launcher Bootstrapper: Game
add_executable(Game src/launcher_main.cpp)
target_include_directories(Game PRIVATE ${ENGINE_INCLUDES})
target_link_libraries(Game PRIVATE DjusovCore)
