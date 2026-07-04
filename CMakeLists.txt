cmake_minimum_required(VERSION 3.20)
project(VoxelEngine VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Allow older sub-project CMakeLists under CMake 4.x
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

include(FetchContent)

# ── GLFW ────────────────────────────────────────────────────────────────────
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.3.9
)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# ── GLM ─────────────────────────────────────────────────────────────────────
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
)
FetchContent_MakeAvailable(glm)

# ── GLAD (v2) ────────────────────────────────────────────────────────────────
FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG        v2.0.8
    SOURCE_SUBDIR  cmake
)
FetchContent_MakeAvailable(glad)
glad_add_library(glad_gl REPRODUCIBLE API gl:core=3.3)

# ── Executable ───────────────────────────────────────────────────────────────
add_executable(voxel-engine
    src/main.cpp
)

target_include_directories(voxel-engine PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(voxel-engine PRIVATE
    glfw
    glm
    glad_gl
    ${CMAKE_DL_LIBS}
)

# Copy shaders next to the executable after each build
add_custom_command(TARGET voxel-engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:voxel-engine>/shaders
    COMMENT "Copying shaders to output directory"
)
