cmake_minimum_required(VERSION 3.16)
project(qr_rubiks_cube CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

add_compile_options(-Wall -Wextra)

# --- Dependencies -----------------------------------------------------------
# GLFW: expects a system install (e.g. `apt install libglfw3-dev`,
# `brew install glfw`, or vcpkg/conan equivalent) discoverable via
# find_package. If your GLFW was installed without CMake config files,
# point CMAKE_PREFIX_PATH at it or adjust this section to use
# pkg_check_modules(glfw3 REQUIRED glfw3) instead.
find_package(glfw3 3.3 REQUIRED)

# GLM: header-only. Most package managers install it such that
# find_package(glm) works (provides the glm::glm interface target). If
# yours doesn't, replace this with an explicit
#   include_directories(/path/to/glm)
# pointing at the directory containing the top-level glm/ folder.
find_package(glm REQUIRED)

# GLAD: NOT fetched automatically (it's a generated loader, not a normal
# library). Generate it for OpenGL 3.3 Core at https://glad.dav1d.de/
# (Language: C/C++, API gl=3.3, Profile: Core, "Generate a loader": yes),
# unzip the result so you have:
#   third_party/glad/include/glad/glad.h
#   third_party/glad/include/KHR/khrplatform.h
#   third_party/glad/src/glad.c
# (Many Linux distros also package a pre-generated glad; if
# third_party/glad doesn't exist this build will fail with a clear "file
# not found" error pointing here.)
set(GLAD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glad)
add_library(glad STATIC ${GLAD_DIR}/src/glad.c)
target_include_directories(glad PUBLIC ${GLAD_DIR}/include)

# --- Main executable ---------------------------------------------------------
add_executable(qr_rubiks_cube
    src/main.cpp
    src/Camera.cpp
    src/Shader.cpp
    src/CubieMeshGenerator.cpp
    src/RubiksCube.cpp
    src/InputController.cpp
)

target_include_directories(qr_rubiks_cube PRIVATE include)

target_link_libraries(qr_rubiks_cube PRIVATE glfw glad glm::glm)

if (UNIX AND NOT APPLE)
    target_link_libraries(qr_rubiks_cube PRIVATE dl)
endif()

# Copy the shader directory next to the built binary so the program's
# relative "shaders/cubie.vert" / "shaders/cubie.frag" paths resolve when
# run from the build directory.
add_custom_command(TARGET qr_rubiks_cube POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/shaders
            $<TARGET_FILE_DIR:qr_rubiks_cube>/shaders
)

# --- Standalone move-logic verification test (no GL/window dependency) ------
# This target builds RubiksCubeTest.cpp against the REAL GLM (via the same
# find_package(glm) above), exercising the exact same RubiksCube.cpp used
# by the main app. (A separate minimal GLM-API-compatible shim under
# test/glm/ was used only to run this same verification in an offline
# environment without GLM installed; it is not used here.)
add_executable(rubiks_cube_test
    test/RubiksCubeTest.cpp
    src/RubiksCube.cpp
)
target_include_directories(rubiks_cube_test PRIVATE include)
target_link_libraries(rubiks_cube_test PRIVATE glm::glm)

enable_testing()
add_test(NAME RubiksCubeMoveLogic COMMAND rubiks_cube_test)
