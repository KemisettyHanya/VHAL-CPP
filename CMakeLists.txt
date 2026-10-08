cmake_minimum_required(VERSION 3.20)
project(can_vhal_studio VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for IDE tooling (Clangd, VS Code, CLion, Visual Studio)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ==============================================================================
# Compiler Warnings & Cross-Platform Definitions (Windows MSVC & Linux GCC/Clang)
# ==============================================================================
if(MSVC)
    add_compile_options(/W4 /utf-8 /permissive-)
    add_compile_definitions(
        _CRT_SECURE_NO_WARNINGS
        _CRT_NONSTDC_NO_DEPRECATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
else()
    add_compile_options(-Wall -Wextra -Wno-unused-parameter)
    if(WIN32)
        add_compile_definitions(
            _CRT_SECURE_NO_WARNINGS
            NOMINMAX
            WIN32_LEAN_AND_MEAN
        )
    endif()
endif()

# ==============================================================================
# Dependency Management: FetchContent with Local Third-Party Fallback (NFR-3)
# ==============================================================================
include(FetchContent)

# 1. Dear ImGui
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/imgui.h")
    set(FETCHCONTENT_SOURCE_DIR_IMGUI "${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui")
endif()
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
)
FetchContent_MakeAvailable(imgui)

# 2. imgui-node-editor
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui-node-editor/imgui_node_editor.h")
    set(FETCHCONTENT_SOURCE_DIR_IMGUI_NODE_EDITOR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui-node-editor")
endif()
FetchContent_Declare(
    imgui_node_editor
    GIT_REPOSITORY https://github.com/thedmd/imgui-node-editor.git
    GIT_TAG master
)
FetchContent_MakeAvailable(imgui_node_editor)

# 3. GLFW & OpenGL
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)

set(GLFW_FOUND FALSE)
# On Linux, use prebuilt Linux .so if present
if(UNIX AND NOT APPLE AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/glfw_prebuilt/lib/libglfw.so")
    message(STATUS "Using prebuilt Linux GLFW from third_party/glfw_prebuilt")
    add_library(glfw SHARED IMPORTED)
    set_target_properties(glfw PROPERTIES
        IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/third_party/glfw_prebuilt/lib/libglfw.so"
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/third_party/glfw_prebuilt/include"
    )
    set(GLFW_FOUND TRUE)
else()
    # On Windows or systems without Linux prebuilt, use third_party/glfw or system/FetchContent
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/glfw/CMakeLists.txt")
        set(FETCHCONTENT_SOURCE_DIR_GLFW "${CMAKE_CURRENT_SOURCE_DIR}/third_party/glfw")
    endif()
    find_package(glfw3 3.3 QUIET)
    if(TARGET glfw)
        set(GLFW_FOUND TRUE)
    endif()
endif()

if(NOT GLFW_FOUND)
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
    )
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(glfw)
endif()

# ==============================================================================
# Third-Party ImGui + Node Editor Library Target
# ==============================================================================
set(IMGUI_DIR "${imgui_SOURCE_DIR}")
set(IMGUI_NODE_EDITOR_DIR "${imgui_node_editor_SOURCE_DIR}")

add_library(imgui_framework STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    ${IMGUI_NODE_EDITOR_DIR}/imgui_node_editor.cpp
    ${IMGUI_NODE_EDITOR_DIR}/imgui_node_editor_api.cpp
    ${IMGUI_NODE_EDITOR_DIR}/imgui_canvas.cpp
    ${IMGUI_NODE_EDITOR_DIR}/crude_json.cpp
)

target_include_directories(imgui_framework PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${IMGUI_NODE_EDITOR_DIR}
)

if(WIN32)
    target_compile_definitions(imgui_framework PUBLIC
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        _CRT_SECURE_NO_WARNINGS
    )
endif()

target_link_libraries(imgui_framework PUBLIC
    glfw
    OpenGL::GL
)

# ==============================================================================
# Core Automotive Engine Library Target
# ==============================================================================
add_library(vhal_core STATIC
    src/core/VhalCatalog.cpp
    src/core/DbcParser.cpp
    src/core/CppReverseParser.cpp
    src/core/CppSynthesizer.cpp
)

target_include_directories(vhal_core PUBLIC
    src
)

if(WIN32)
    target_compile_definitions(vhal_core PUBLIC
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        _CRT_SECURE_NO_WARNINGS
    )
endif()

# ==============================================================================
# Main Executable: can_vhal_studio
# ==============================================================================
add_executable(can_vhal_studio
    src/main.cpp
    src/ui/AppWindow.cpp
    src/ui/CanvasView.cpp
    src/ui/SignalPalette.cpp
    src/ui/PropertyPalette.cpp
)

target_include_directories(can_vhal_studio PRIVATE
    src
)

target_link_libraries(can_vhal_studio PRIVATE
    vhal_core
    imgui_framework
    glfw
    OpenGL::GL
    Threads::Threads
)

# Linux-specific dynamic linking and RPATH
if(UNIX AND NOT APPLE)
    target_link_libraries(can_vhal_studio PRIVATE
        ${CMAKE_DL_LIBS}
    )
    set_target_properties(can_vhal_studio PROPERTIES
        INSTALL_RPATH "$ORIGIN/third_party/glfw_prebuilt/lib:${CMAKE_CURRENT_SOURCE_DIR}/third_party/glfw_prebuilt/lib"
        BUILD_WITH_INSTALL_RPATH TRUE
    )
endif()

# ==============================================================================
# Tests Executable: can_vhal_tests
# ==============================================================================
add_executable(can_vhal_tests
    tests/unit_tests.cpp
)

target_include_directories(can_vhal_tests PRIVATE
    src
)

target_link_libraries(can_vhal_tests PRIVATE
    vhal_core
    Threads::Threads
)

enable_testing()
add_test(NAME CoreAndBenchmarkTests COMMAND can_vhal_tests)
set_tests_properties(CoreAndBenchmarkTests PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

