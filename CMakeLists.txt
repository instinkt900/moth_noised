cmake_minimum_required(VERSION 3.27)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/version.txt" MOTH_NOISED_VERSION_FULL)
string(STRIP "${MOTH_NOISED_VERSION_FULL}" MOTH_NOISED_VERSION_FULL)
string(REGEX REPLACE "[-+].*$" "" MOTH_NOISED_VERSION "${MOTH_NOISED_VERSION_FULL}")

project(moth_noised
    VERSION ${MOTH_NOISED_VERSION}
    DESCRIPTION "A node editor for moth::noise noise graphs"
    LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# No global CMAKE_CXX_STANDARD on purpose. The fetched UI stack sets its own
# (imnodes drops it to 11 for everything defined after it), so a global value
# here is not something the editor target can rely on. Each target that needs a
# standard asks for it directly instead.

include(GNUInstallDirs)
include(cmake/CPM.cmake)

# ---------------------------------------------------------------------------
# Dependencies come from two places, on purpose.
#
# Conan supplies the data half: moth::noise (the JSON node-graph format this
# editor reads and writes) and FastNoise2 underneath it. Taking FastNoise2
# through Conan rather than building it here means the editor and any engine
# reading its files link the exact same library revision, so a graph that
# round-trips in the editor round-trips in the game.
#
# CPM supplies the UI half — Magnum, ImGui, imnodes — which is what upstream's
# editor does and which has no Conan packaging worth adopting (imnodes in
# particular is a fork pinned by commit). Those live in src/CMakeLists.txt
# next to the target that uses them.
#
# moth::noise is built against moth_core with enable_platform=False, so nothing
# in the Conan half pulls in a windowing stack to collide with the GLFW that
# Magnum brings.
# ---------------------------------------------------------------------------
find_package(moth_noise CONFIG REQUIRED)
# The version is requested rather than left open so that CMake populates
# FastNoise2_VERSION, which src/CMakeLists.txt needs: upstream's editor reads
# FASTNOISE2_VERSION out of a definition the FastNoise target carries, and the
# packaged library does not carry it.
find_package(FastNoise2 1.1 CONFIG REQUIRED)

add_subdirectory(ipc)
add_subdirectory(src)
