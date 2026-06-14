cmake_minimum_required(VERSION 3.12)

if (POLICY CMP0144)
    cmake_policy(SET CMP0144 NEW)
endif ()

project(CKAngelScript VERSION 0.1.0.0)

option(CKAS_BUILD_SELF_TESTS "Build optional AngelScript startup self-test support" OFF)
option(CKAS_ENABLE_DYNCALL "Build DynCall-backed native FFI support" OFF)
option(CKAS_INSTALL_DOCS "Install README, license, and Markdown documentation" ON)

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

if (NOT SKIP_INSTALL_ALL AND CKAS_INSTALL_DOCS)
    install(FILES
            "${PROJECT_SOURCE_DIR}/README.md"
            "${PROJECT_SOURCE_DIR}/LICENSE"
            DESTINATION "${CMAKE_INSTALL_DOCDIR}")
    install(DIRECTORY "${PROJECT_SOURCE_DIR}/docs/"
            DESTINATION "${CMAKE_INSTALL_DOCDIR}/docs"
            FILES_MATCHING PATTERN "*.md")
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
