cmake_minimum_required(VERSION 3.25)

if (POLICY CMP0144)
    cmake_policy(SET CMP0144 NEW)
endif ()

project(CKAngelScript VERSION 0.5.0)

if (PROJECT_VERSION_TWEAK STREQUAL "")
    set(CKAS_VERSION_TWEAK_VALUE 0)
else ()
    set(CKAS_VERSION_TWEAK_VALUE ${PROJECT_VERSION_TWEAK})
endif ()

option(CKAS_BUILD_SELF_TESTS "Build optional AngelScript startup self-test support" OFF)
option(CKAS_BUILD_CORE_TESTS "Build host-independent core tests" ON)
option(CKAS_ENABLE_DYNCALL "Build DynCall-backed native FFI support" OFF)
option(CKAS_ENABLE_API_EXPORT "Build runtime AngelScript API JSON export support" OFF)
option(CKAS_INSTALL_DOCS "Install README, license, and Markdown documentation" ON)
option(CKAS_BUILD_API_DOCS "Enable the docs-api target for generated Natural Docs API reference" OFF)
set(CKAS_NATURALDOCS_EXECUTABLE "" CACHE FILEPATH "Path to the Natural Docs executable used by docs-api")
set(CKAS_SCRIPT_API_JSON_INPUT "" CACHE FILEPATH "Host-exported AngelScript API JSON used by docs-api")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS YES)

# Add path for custom modules
list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(GenerateExportHeader)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Use folders to organize targets in an IDE
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMakeTargets")

set(CKAS_RUNTIME sdl CACHE STRING "Engine runtime")
set_property(CACHE CKAS_RUNTIME PROPERTY STRINGS sdl virtools)
if(CKAS_RUNTIME STREQUAL "sdl")
    set(CKAS_RUNTIME_SDL 1)
    if(VIRTOOLS_SDK_PATH OR VIRTOOLS_SDK_INCLUDE_DIR)
        message(FATAL_ERROR "Use NeMo2's CK2 and VxMath packages and a clean SDL build directory.")
    endif()
elseif(CKAS_RUNTIME STREQUAL "virtools")
    set(CKAS_RUNTIME_SDL 0)
    if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 4
            OR NOT CMAKE_CXX_COMPILER_ARCHITECTURE_ID STREQUAL "X86")
        message(FATAL_ERROR "Virtools requires Windows x86 with the MSVC ABI (VS 2022: -A Win32).")
    endif()
else()
    message(FATAL_ERROR "CKAS_RUNTIME must be sdl or virtools.")
endif()
add_compile_definitions(CKAS_RUNTIME_SDL=${CKAS_RUNTIME_SDL})
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

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

if(CKAS_RUNTIME_SDL)
    # An embedding project may already supply the engine used by its Player.
    if(NOT TARGET SDL3::SDL3)
        find_package(SDL3 3.4.8 CONFIG REQUIRED)
    endif()
    if(NOT TARGET VxMath AND NOT TARGET VxMath::VxMath)
        find_package(VxMath CONFIG REQUIRED)
    endif()
    if(NOT TARGET VxMath AND TARGET VxMath::VxMath)
        add_library(VxMath ALIAS VxMath::VxMath)
    endif()
    if(NOT TARGET CK2)
        find_package(CK2 CONFIG REQUIRED)
    endif()
else()
    find_package(VirtoolsSDK MODULE REQUIRED)
    set(CKAS_ENGINE_TEST_RUNTIME_DIR "" CACHE PATH
        "Virtools test directory containing the original CK2.dll and VxMath.dll")
endif()

if (CKAS_ENABLE_DYNCALL)
    find_package(DynCall REQUIRED)
    find_package(DynCallback REQUIRED)
    find_package(DynLoad REQUIRED)
endif ()

add_subdirectory(deps)
add_subdirectory(src)
if (CKAS_BUILD_CORE_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif ()
#add_subdirectory(tools)

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
    if (NOT CKAS_SCRIPT_API_JSON_INPUT)
        message(FATAL_ERROR "CKAS_BUILD_API_DOCS requires CKAS_SCRIPT_API_JSON_INPUT to point at an API JSON exported by a host integration.")
    endif ()
    find_package(Python3 REQUIRED COMPONENTS Interpreter)

    add_custom_command(
            OUTPUT "${CKAS_API_OUTPUT_DIR}/index.html"
            COMMAND "${Python3_EXECUTABLE}"
                    "${PROJECT_SOURCE_DIR}/tools/generate_naturaldocs_api.py"
                    --api-json "${CKAS_SCRIPT_API_JSON_INPUT}"
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
                    "${CKAS_SCRIPT_API_JSON_INPUT}"
                    "${PROJECT_SOURCE_DIR}/tools/generate_naturaldocs_api.py"
                    "${PROJECT_SOURCE_DIR}/docs/api-reference.yml"
                    "${PROJECT_SOURCE_DIR}/docs/naturaldocs/Project.txt"
                    "${PROJECT_SOURCE_DIR}/docs/naturaldocs/Languages.txt"
                    "${PROJECT_SOURCE_DIR}/docs/naturaldocs/Comments.txt"
            BYPRODUCTS
                    "${CKAS_API_QUALITY_JSON}"
            COMMENT "Generating CKAngelScript Natural Docs API reference"
            VERBATIM
    )
    add_custom_target(docs-api DEPENDS "${CKAS_API_OUTPUT_DIR}/index.html")
else ()
    add_custom_target(docs-api
            COMMAND "${CMAKE_COMMAND}" -E echo "Reconfigure with -DCKAS_BUILD_API_DOCS=ON, -DCKAS_NATURALDOCS_EXECUTABLE=<path>, and -DCKAS_SCRIPT_API_JSON_INPUT=<host-export.json> to enable docs-api."
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
set(_ckas_arch "${CMAKE_SYSTEM_PROCESSOR}")
if(CMAKE_OSX_ARCHITECTURES)
    set(_ckas_arch "${CMAKE_OSX_ARCHITECTURES}")
elseif(CMAKE_VS_PLATFORM_NAME)
    set(_ckas_arch "${CMAKE_VS_PLATFORM_NAME}")
elseif(CMAKE_CXX_COMPILER_ARCHITECTURE_ID)
    set(_ckas_arch "${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}")
endif()
string(TOLOWER "${_ckas_arch}" _ckas_arch)
if(_ckas_arch MATCHES "^(arm64|aarch64)$")
    set(_ckas_arch arm64)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4 AND WIN32 AND _ckas_arch MATCHES "^(win32|x86|i[3-6]86|amd64|x86_64)$")
    set(_ckas_arch x86)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8 AND _ckas_arch MATCHES "^(amd64|x64|x86_64)$")
    set(_ckas_arch x64)
else()
    message(FATAL_ERROR "Unsupported CKAngelScript architecture: ${_ckas_arch}. Build one architecture per SDK.")
endif()
string(TOLOWER "${CMAKE_SYSTEM_NAME}" _ckas_platform)
if(APPLE)
    set(_ckas_platform macos)
endif()
configure_file(cmake/CKAngelScript.runtime.json.in CKAngelScript.runtime.json @ONLY)
if(NOT SKIP_INSTALL_ALL)
    install(FILES "${PROJECT_BINARY_DIR}/CKAngelScript.runtime.json"
            DESTINATION "${CMAKE_INSTALL_DATADIR}/CKAngelScript")
endif()
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${CKAS_RUNTIME}-${_ckas_platform}-${_ckas_arch}")
set(CPACK_GENERATOR "ZIP")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY ON)
set(CPACK_PACKAGING_INSTALL_PREFIX "")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")
include(CPack)
