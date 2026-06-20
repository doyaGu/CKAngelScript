cmake_minimum_required(VERSION 3.12)

if (POLICY CMP0144)
    cmake_policy(SET CMP0144 NEW)
endif ()

project(CKAngelScript VERSION 0.2.0)

if (CMAKE_PROJECT_VERSION_TWEAK STREQUAL "")
    set(CKAS_VERSION_TWEAK_VALUE 0)
else ()
    set(CKAS_VERSION_TWEAK_VALUE ${CMAKE_PROJECT_VERSION_TWEAK})
endif ()

option(CKAS_BUILD_SELF_TESTS "Build optional AngelScript startup self-test support" OFF)
option(CKAS_ENABLE_DYNCALL "Build DynCall-backed native FFI support" OFF)
option(CKAS_ENABLE_API_EXPORT "Build runtime AngelScript API JSON export support" OFF)
option(CKAS_INSTALL_DOCS "Install README, license, and Markdown documentation" ON)
option(CKAS_BUILD_API_DOCS "Enable the docs-api target for generated Natural Docs API reference" OFF)
set(CKAS_NATURALDOCS_EXECUTABLE "" CACHE FILEPATH "Path to the Natural Docs executable used by docs-api")
set(CKAS_DOCS_BALLANCE_ROOT "" CACHE PATH "Optional Ballance root used by docs-api runtime export")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED NO)
set(CMAKE_CXX_EXTENSIONS YES)

# Add path for custom modules
list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(GenerateExportHeader)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Use folders to organize targets in an IDE
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMakeTargets")

if (NOT WIN32)
    message(FATAL_ERROR "Only support Windows.")
endif ()

# Use relative paths
if (WIN32)
    set(CMAKE_USE_RELATIVE_PATHS TRUE)
    set(CMAKE_SUPPRESS_REGENERATION TRUE)
endif ()

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Release' as no build type was specified")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the build type (Debug/Release)" FORCE)
endif ()

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/install" CACHE PATH "The root directory of the installation" FORCE)
    message(STATUS "Setting default install directory to ${CMAKE_INSTALL_PREFIX} as no install directory was specified")
endif ()

# Disable msvc unsafe warnings
add_compile_definitions(
        $<$<C_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
        $<$<C_COMPILER_ID:MSVC>:_CRT_NONSTDC_NO_WARNINGS>
)

find_package(VirtoolsSDK REQUIRED)
if (CKAS_ENABLE_DYNCALL)
    find_package(DynCall REQUIRED)
    find_package(DynCallback REQUIRED)
    find_package(DynLoad REQUIRED)
endif ()

add_subdirectory(deps)
add_subdirectory(src)
#add_subdirectory(tools)

set(CKAS_SCRIPT_API_JSON "${CMAKE_BINARY_DIR}/docs/script-api.json")
set(CKAS_API_SOURCE_DIR "${CMAKE_BINARY_DIR}/docs/api-source")
set(CKAS_API_OUTPUT_DIR "${CMAKE_BINARY_DIR}/docs/api")
set(CKAS_API_QUALITY_JSON "${CMAKE_BINARY_DIR}/docs/api-quality.json")

if (CKAS_BUILD_API_DOCS)
    if (NOT CKAS_ENABLE_API_EXPORT)
        message(FATAL_ERROR "CKAS_BUILD_API_DOCS requires CKAS_ENABLE_API_EXPORT=ON.")
    endif ()
    if (NOT CKAS_NATURALDOCS_EXECUTABLE)
        message(FATAL_ERROR "CKAS_BUILD_API_DOCS requires CKAS_NATURALDOCS_EXECUTABLE to point at NaturalDocs.exe.")
    endif ()
    find_package(Python3 REQUIRED COMPONENTS Interpreter)

    add_custom_command(
            OUTPUT "${CKAS_API_OUTPUT_DIR}/index.html"
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/docs"
            COMMAND powershell
                    -NoProfile
                    -ExecutionPolicy Bypass
                    -File "${PROJECT_SOURCE_DIR}/tools/Validate-Ballance.ps1"
                    -BallanceRoot "${CKAS_DOCS_BALLANCE_ROOT}"
                    -BuildDll "$<TARGET_FILE:CKAngelScript>"
                    -ValidationDir "${CMAKE_BINARY_DIR}/validation/ballance"
                    -ExportScriptApi "${CKAS_SCRIPT_API_JSON}"
            COMMAND "${Python3_EXECUTABLE}"
                    "${PROJECT_SOURCE_DIR}/tools/generate_naturaldocs_api.py"
                    --api-json "${CKAS_SCRIPT_API_JSON}"
                    --metadata "${PROJECT_SOURCE_DIR}/docs/api-reference.yml"
                    --out-dir "${CKAS_API_SOURCE_DIR}"
                    --quality-json "${CKAS_API_QUALITY_JSON}"
            COMMAND "${CKAS_NATURALDOCS_EXECUTABLE}"
                    "${PROJECT_SOURCE_DIR}/docs/naturaldocs"
                    --rebuild
            COMMAND "${CMAKE_COMMAND}" -E compare_files
                    "${CKAS_API_OUTPUT_DIR}/index.html"
                    "${CKAS_API_OUTPUT_DIR}/index.html"
            DEPENDS
                    CKAngelScript
                    "${PROJECT_SOURCE_DIR}/tools/Validate-Ballance.ps1"
                    "${PROJECT_SOURCE_DIR}/tools/generate_naturaldocs_api.py"
                    "${PROJECT_SOURCE_DIR}/docs/api-reference.yml"
                    "${PROJECT_SOURCE_DIR}/docs/naturaldocs/Project.txt"
                    "${PROJECT_SOURCE_DIR}/docs/naturaldocs/Languages.txt"
                    "${PROJECT_SOURCE_DIR}/docs/naturaldocs/Comments.txt"
            BYPRODUCTS
                    "${CKAS_SCRIPT_API_JSON}"
                    "${CKAS_API_QUALITY_JSON}"
            COMMENT "Generating CKAngelScript Natural Docs API reference"
            VERBATIM
    )
    add_custom_target(docs-api DEPENDS "${CKAS_API_OUTPUT_DIR}/index.html")
else ()
    add_custom_target(docs-api
            COMMAND "${CMAKE_COMMAND}" -E echo "Reconfigure with -DCKAS_BUILD_API_DOCS=ON and -DCKAS_NATURALDOCS_EXECUTABLE=<path> to enable docs-api."
            COMMAND "${CMAKE_COMMAND}" -E false
            VERBATIM)
endif ()

if (NOT SKIP_INSTALL_ALL AND CKAS_INSTALL_DOCS)
    install(FILES
            "${PROJECT_SOURCE_DIR}/README.md"
            "${PROJECT_SOURCE_DIR}/LICENSE"
            DESTINATION "${CMAKE_INSTALL_DOCDIR}")
    install(DIRECTORY "${PROJECT_SOURCE_DIR}/docs/"
            DESTINATION "${CMAKE_INSTALL_DOCDIR}/docs"
            FILES_MATCHING PATTERN "*.md")
    install(DIRECTORY "${CKAS_API_OUTPUT_DIR}/"
            DESTINATION "${CMAKE_INSTALL_DOCDIR}/api"
            OPTIONAL)
endif ()

set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VENDOR "CKAngelScript")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "AngelScript integration for Virtools/CK2")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-win32")
set(CPACK_GENERATOR "ZIP")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY ON)
set(CPACK_PACKAGING_INSTALL_PREFIX "")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")
include(CPack)
