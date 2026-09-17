# ---------------------------------------------------------------------------
# The UI stack, fetched by CPM.
#
# This block is upstream FastNoise2's, kept close to verbatim: the pins are
# load-bearing (imnodes is a fork carried by commit, Magnum and Corrade must
# agree with each other) and there is nothing to gain from re-deriving them.
# What is not here is FastNoise2 itself — the root CMakeLists takes that from
# Conan so the editor and the engine share one build of it.
# ---------------------------------------------------------------------------
CPMAddPackage(
    NAME corrade
    GITHUB_REPOSITORY mosra/corrade
    GIT_TAG e716388dd792d90d08b0936d460f6ccdb34150de
    GIT_SUBMODULES "src"
    EXCLUDE_FROM_ALL YES
    OPTIONS
        "CORRADE_BUILD_STATIC ON"
        "CORRADE_BUILD_STATIC_UNIQUE_GLOBALS OFF"
        "CORRADE_MSVC_COMPATIBILITY ON"
        "CORRADE_WITH_INTERCONNECT OFF"
        "CORRADE_WITH_TESTSUITE OFF"
)

if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    set(NODE_EDITOR_APP_TYPE_CAPS "EMSCRIPTEN")
    set(NODE_EDITOR_APP_TYPE "Emscripten")
else()
    set(NODE_EDITOR_APP_TYPE_CAPS "GLFW")
    set(NODE_EDITOR_APP_TYPE "Glfw")

    CPMAddPackage(
        NAME GLFW
        GITHUB_REPOSITORY glfw/glfw
        GIT_TAG 3.4
        EXCLUDE_FROM_ALL YES
        OPTIONS
            "BUILD_SHARED_LIBS OFF"
            "GLFW_INSTALL OFF"
            "GLFW_BUILD_TESTS OFF"
            "GLFW_BUILD_EXAMPLES OFF"
            "GLFW_BUILD_DOCS OFF"
    )
endif()

CPMAddPackage(
    NAME magnum
    GITHUB_REPOSITORY mosra/magnum
    GIT_TAG 179a68229f57bd0f833dad30647f2988fd8d8966
    GIT_SUBMODULES "src"
    EXCLUDE_FROM_ALL YES
    OPTIONS
        "MAGNUM_BUILD_STATIC ON"
        "MAGNUM_BUILD_PLUGINS_STATIC ON"
        "MAGNUM_BUILD_STATIC_UNIQUE_GLOBALS OFF"
        "MAGNUM_WITH_${NODE_EDITOR_APP_TYPE_CAPS}APPLICATION ON"
        "MAGNUM_WITH_MESHTOOLS OFF"
        "MAGNUM_WITH_TRADE OFF"
        "MAGNUM_WITH_TEXT OFF"
        "MAGNUM_WITH_TEXTURETOOLS OFF"
        "MAGNUM_TARGET_GLES2 OFF"
)

CPMAddPackage(
    NAME imgui
    GITHUB_REPOSITORY ocornut/imgui
    GIT_TAG v1.90.1-docking
    EXCLUDE_FROM_ALL YES
    DOWNLOAD_ONLY YES
)
# Where find_package(ImGui) looks.
set(IMGUI_DIR ${imgui_SOURCE_DIR})

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    CPMAddPackage(
        NAME freetype
        GIT_REPOSITORY https://gitlab.freedesktop.org/freetype/freetype.git
        GIT_TAG VER-2-14-1
        EXCLUDE_FROM_ALL YES
        OPTIONS
            "BUILD_SHARED_LIBS OFF"
            "FT_REQUIRE_ZLIB OFF"
            "FT_DISABLE_ZLIB ON"
            "FT_REQUIRE_BZIP2 OFF"
            "FT_DISABLE_BZIP2 ON"
            "FT_REQUIRE_PNG OFF"
            "FT_DISABLE_PNG ON"
            "FT_REQUIRE_BROTLI OFF"
            "FT_DISABLE_BROTLI ON"
            "FT_REQUIRE_HARFBUZZ OFF"
            "FT_DISABLE_HARFBUZZ ON"
    )
endif()

CPMAddPackage(
    NAME magnum-integration
    GITHUB_REPOSITORY mosra/magnum-integration
    GIT_TAG 354f1295b91b14f86edd18b1574567d83cad5704
    GIT_SUBMODULES "src"
    EXCLUDE_FROM_ALL YES
    OPTIONS
        "BUILD_STATIC ON"
        "MAGNUM_WITH_IMGUIINTEGRATION ON"
)

# magnum-integration ships find modules for Magnum and ImGui both.
set(CMAKE_MODULE_PATH "${magnum-integration_SOURCE_DIR}/modules" ${CMAKE_MODULE_PATH})

find_package(Magnum REQUIRED GL ${NODE_EDITOR_APP_TYPE}Application)
find_package(MagnumIntegration REQUIRED ImGui)
find_package(ImGui REQUIRED SourcesMiscCpp)

# IMGUI_ENABLE_FREETYPE has to reach every target that compiles an ImGui
# source, so it goes PUBLIC on the target that actually owns those sources.
target_compile_definitions(MagnumImGuiIntegration PUBLIC IMGUI_ENABLE_FREETYPE)

if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    target_compile_options(MagnumImGuiIntegration PUBLIC -sUSE_FREETYPE=1)
    target_link_options(MagnumImGuiIntegration PUBLIC -sUSE_FREETYPE=1)
else()
    cmake_policy(SET CMP0079 NEW)
    target_link_libraries(MagnumImGuiIntegration PUBLIC freetype)
endif()

set(IMGUI_FREETYPE_SOURCE "${imgui_SOURCE_DIR}/misc/freetype/imgui_freetype.cpp")
set_property(SOURCE ${IMGUI_FREETYPE_SOURCE} APPEND PROPERTY COMPILE_DEFINITIONS
    "IMGUI_USER_CONFIG=\"Magnum/ImGuiIntegration/visibility.h\"")

CPMAddPackage(
    NAME imnodes
    GITHUB_REPOSITORY Auburn/imnodes
    GIT_TAG db2ef1192a4ddff32a838094de7127142a731ef0 #zoom2
    GIT_SUBMODULES ".github"
    EXCLUDE_FROM_ALL YES
    OPTIONS
        "BUILD_SHARED_LIBS OFF"
        "IMNODES_IMGUI_TARGET_NAME MagnumIntegration::ImGui"
)

# Not upstream's: the editor gains named project files, so it needs a real
# open/save dialog. On Linux this builds against GTK3, the same way moth_editor
# does; NFD_PORTAL=ON is the alternative if a desktop portal is preferred.
CPMAddPackage(
    NAME nativefiledialog-extended
    GITHUB_REPOSITORY btzy/nativefiledialog-extended
    GIT_TAG v1.2.1
    EXCLUDE_FROM_ALL YES
    OPTIONS
        "NFD_BUILD_TESTS OFF"
        "NFD_INSTALL OFF"
)

CPMAddPackage(
    NAME robinhoodhashing
    GITHUB_REPOSITORY martinus/robin-hood-hashing
    GIT_TAG 3.11.5
    EXCLUDE_FROM_ALL YES
)

# ---------------------------------------------------------------------------
# The editor
# ---------------------------------------------------------------------------

# The font is pulled out of the ImGui checkout, so the resource manifest has to
# be configured rather than used as-is.
set(NodeEditor_RESOURCES_DIR "${CMAKE_CURRENT_LIST_DIR}/resources")
configure_file("resources/resources.conf" "${CMAKE_CURRENT_BINARY_DIR}/resources.conf")
corrade_add_resource(NodeEditor_RESOURCES "${CMAKE_CURRENT_BINARY_DIR}/resources.conf")

add_executable(moth_noised
    Project.cpp
    NodeEditorApp.cpp
    FastNoiseNodeEditor.cpp
    MeshNoisePreview.cpp
    NoiseTexture.cpp
    ${IMGUI_FREETYPE_SOURCE}
    ${NodeEditor_RESOURCES}
)

target_link_libraries(moth_noised PRIVATE
    moth::noise
    FastNoise2::FastNoise
    moth_noised_ipc
    Magnum::Application
    Magnum::Shaders
    Magnum::SceneGraph
    MagnumIntegration::ImGui
    ImGui::SourcesMiscCpp
    imnodes
    robin_hood
    nfd
)

# Upstream's FastNoise target defines this PUBLIC from the FastNoise2 project
# version, and the About box prints it. The packaged library carries no such
# definition, so supply it here from what find_package reported.
target_compile_definitions(moth_noised PRIVATE FASTNOISE2_VERSION="${FastNoise2_VERSION}")

# Directly, rather than via CMAKE_CXX_STANDARD: imnodes sets that variable to 11
# in its own CMakeLists, which lands on every target configured after it.
target_compile_features(moth_noised PRIVATE cxx_std_20)

if(CORRADE_TARGET_WINDOWS)
    target_sources(moth_noised PRIVATE resources/WindowsHiDPI.manifest)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    target_compile_options(moth_noised PRIVATE -pthread -msimd128)
    target_link_options(moth_noised PRIVATE
        "-sPTHREAD_POOL_SIZE=Math.max(2,navigator.hardwareConcurrency)+3-navigator.hardwareConcurrency/4"
        -pthread -sALLOW_MEMORY_GROWTH=1 -lidbfs.js -s FORCE_FILESYSTEM
        --shell-file "${CMAKE_CURRENT_LIST_DIR}/resources/emscripten_shell.html"
        --pre-js "${CMAKE_CURRENT_LIST_DIR}/resources/emscripten_pre.js"
        -Wl,-u,_emscripten_run_callback_on_thread
    )
    add_custom_command(TARGET moth_noised POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_LIST_DIR}/resources/emscripten_enable_shared_array_buffer.js"
        $<TARGET_FILE_DIR:moth_noised>
    )
    install(FILES
        $<TARGET_FILE_DIR:moth_noised>/moth_noised.js
        $<TARGET_FILE_DIR:moth_noised>/moth_noised.wasm
        "${CMAKE_CURRENT_LIST_DIR}/resources/emscripten_enable_shared_array_buffer.js"
        DESTINATION "${CMAKE_INSTALL_BINDIR}"
    )
elseif(UNIX)
    target_link_options(moth_noised PRIVATE -pthread)

    if(APPLE)
        set_property(TARGET moth_noised PROPERTY INSTALL_RPATH "@loader_path/../lib")
    else()
        set_property(TARGET moth_noised PROPERTY INSTALL_RPATH "\$ORIGIN/../lib")
    endif()
endif()

if(MSVC)
    target_compile_definitions(moth_noised PRIVATE _CRT_SECURE_NO_WARNINGS=1)
endif()

install(TARGETS moth_noised RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

# Shared libraries the executable loads at run time on Windows: the DLLs the
# recipe collects from Conan dependencies into MOTH_NOISED_RUNTIME_DIR (FastNoise2)
# and moth_noised_ipc when it is built shared.
# They are copied next to the executable after each build, so it runs from the
# build tree, and installed beside it.
if(WIN32 AND MOTH_NOISED_RUNTIME_DIR)
    file(GLOB MOTH_NOISED_RUNTIME_DLLS "${MOTH_NOISED_RUNTIME_DIR}/*.dll")
    if(MOTH_NOISED_FASTNOISE_SHARED)
        list(APPEND MOTH_NOISED_RUNTIME_DLLS $<TARGET_FILE:moth_noised_ipc>)
    endif()
    if(MOTH_NOISED_RUNTIME_DLLS)
        add_custom_command(TARGET moth_noised POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${MOTH_NOISED_RUNTIME_DLLS} $<TARGET_FILE_DIR:moth_noised>
            VERBATIM
        )
        install(FILES ${MOTH_NOISED_RUNTIME_DLLS} DESTINATION "${CMAKE_INSTALL_BINDIR}")
    endif()
endif()

set_property(DIRECTORY "${CMAKE_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT moth_noised)
