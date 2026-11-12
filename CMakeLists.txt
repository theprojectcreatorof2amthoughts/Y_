cmake_minimum_required(VERSION 3.16)
project(Y_ VERSION 0.3.0 LANGUAGES C)

include(GNUInstallDirs)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

file(GLOB_RECURSE YL_LIBRARIES
    CONFIGURE_DEPENDS
    LIST_DIRECTORIES false
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hly"
)
list(FILTER YL_LIBRARIES EXCLUDE REGEX "(^|/)\\.git(/|$)")
list(FILTER YL_LIBRARIES EXCLUDE REGEX "(^|/)(build|cmake-build[^/]*)/")
list(SORT YL_LIBRARIES)

set(GENERATED_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(EMBEDDER_EXECUTABLE "${CMAKE_CURRENT_BINARY_DIR}/embed_hly")
set(EMBEDDED_SOURCE "${GENERATED_DIRECTORY}/embedded_libraries.c")

add_executable(hly_embedder EXCLUDE_FROM_ALL tools/embed_hly.c)
set_target_properties(hly_embedder PROPERTIES
    OUTPUT_NAME embed_hly
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

add_custom_command(
    OUTPUT "${EMBEDDED_SOURCE}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${GENERATED_DIRECTORY}"
    COMMAND "$<TARGET_FILE:hly_embedder>" "${EMBEDDED_SOURCE}" ${YL_LIBRARIES}
    DEPENDS hly_embedder ${YL_LIBRARIES} src/embedded_libraries.h
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    VERBATIM
    COMMENT "Embedding Y_ HLY libraries"
)
set_source_files_properties("${EMBEDDED_SOURCE}" PROPERTIES GENERATED TRUE)

add_executable(y_
    src/main.c
    src/y_.c
    src/memory.c
    src/file_io.c
    src/embedded_libraries.h
    "${EMBEDDED_SOURCE}"
)
target_include_directories(y_ PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_options(y_ PRIVATE -Wall -Wextra -Wpedantic)
if(NOT WIN32)
    target_link_libraries(y_ PRIVATE m)
endif()

install(TARGETS y_ RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

set(CPACK_PACKAGE_NAME "y-language")
set(CPACK_PACKAGE_VENDOR "Y_ Project")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Y_ interpreter with embedded standard libraries")
if(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "DEB;TGZ")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Y_ Project")
    set(CPACK_DEBIAN_PACKAGE_SECTION "interpreters")
else()
    set(CPACK_GENERATOR "TGZ")
endif()
include(CPack)
