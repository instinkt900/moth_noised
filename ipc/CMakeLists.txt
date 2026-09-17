# The shared-memory channel a running game uses to read the node tree the
# editor currently has selected. Vendored from FastNoise2 v1.1.1.
#
# Upstream built this against the FastNoise2 source tree; here it takes the
# one header it needs (FastNoise/Utility/Export.h) from the installed package.

add_library(moth_noised_ipc STATIC
    NodeEditorIpc_C.cpp
    include/FastNoise/NodeEditorIpc_C.h
)
target_compile_features(moth_noised_ipc PUBLIC cxx_std_17)

target_include_directories(moth_noised_ipc PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# For FastNoise/Utility/Export.h only — no FastNoise symbols are referenced,
# so this is a header dependency rather than a link one.
target_link_libraries(moth_noised_ipc PUBLIC FastNoise2::FastNoise)

# FastNoise2 is packaged static, so the export header must not decorate its
# declarations with dllimport.
target_compile_definitions(moth_noised_ipc PRIVATE FASTNOISE_STATIC_LIB)

# shm_open lives in librt on older glibc.
if(UNIX AND NOT APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    target_link_libraries(moth_noised_ipc PRIVATE rt)
endif()

install(
    DIRECTORY include/FastNoise
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    FILES_MATCHING PATTERN "*.h"
)
