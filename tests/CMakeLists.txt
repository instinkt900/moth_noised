cmake_minimum_required(VERSION 3.27)

project(moth_noised_tests VERSION 1.0 DESCRIPTION "Unit tests for the moth_noised project format")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Catch2 3 REQUIRED)
find_package(moth_noise CONFIG REQUIRED)

# The project format only, compiled straight out of the editor sources. Keeping
# this build separate from the editor's means the format can be tested without
# configuring the UI stack, which is what makes it cheap enough to run often.
set(SOURCES
    src/main.cpp
    src/test_project.cpp
    ../src/Project.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)
target_link_libraries(${PROJECT_NAME} PRIVATE moth::noise Catch2::Catch2)

if(MSVC)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(${PROJECT_NAME} PRIVATE /W3 /WX /utf-8)
endif()

# Shared libraries the executable loads at run time on Windows: the DLLs the
# recipe collects from Conan dependencies into MOTH_NOISED_RUNTIME_DIR (FastNoise2).
# They are copied next to the test executable after each build, so it runs from
# the build tree.
if(WIN32 AND MOTH_NOISED_RUNTIME_DIR)
    file(GLOB MOTH_NOISED_TESTS_RUNTIME_DLLS "${MOTH_NOISED_RUNTIME_DIR}/*.dll")
    if(MOTH_NOISED_TESTS_RUNTIME_DLLS)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${MOTH_NOISED_TESTS_RUNTIME_DLLS} $<TARGET_FILE_DIR:${PROJECT_NAME}>
            VERBATIM
        )
    endif()
endif()
