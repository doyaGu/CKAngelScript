# AngelScript
set(CKAS_ANGELSCRIPT_VERSION "2.39.0" CACHE STRING "AngelScript SDK version to download when deps/angelscript is missing")
set(CKAS_ANGELSCRIPT_ROOT
        "${CMAKE_CURRENT_SOURCE_DIR}/angelscript"
        CACHE PATH "AngelScript SDK source root")
set(CKAS_ANGELSCRIPT_URL
        ""
        CACHE STRING "AngelScript SDK archive URL; empty uses the AngelCode URL for CKAS_ANGELSCRIPT_VERSION")
set(CKAS_ANGELSCRIPT_SHA256
        "f029bea4e52c360c750d3a805b80f9317d177241fadbc8f93845e38355d11287"
        CACHE STRING "Expected SHA256 for the AngelScript SDK archive")
set(CKAS_ANGELSCRIPT_ARCHIVE "" CACHE FILEPATH "Existing AngelScript SDK archive to use instead of downloading")
set(CKAS_ANGELSCRIPT_DOWNLOAD_DIR
        "${CMAKE_CURRENT_SOURCE_DIR}/_downloads"
        CACHE PATH "Directory used for downloaded dependency archives")
set(CKAS_ANGELSCRIPT_TLS_VERIFY OFF CACHE BOOL "Verify TLS certificates when downloading the AngelScript SDK")

set(CKAS_ANGELSCRIPT_SENTINEL "${CKAS_ANGELSCRIPT_ROOT}/angelscript/include/angelscript.h")

function(ckas_check_angelscript_version header)
    file(STRINGS "${header}" version_string
            REGEX "^#define[ 	]+ANGELSCRIPT_VERSION_STRING[ 	]+" LIMIT_COUNT 1)
    string(REGEX MATCH [["([^"]+)"]] match "${version_string}")
    if (NOT "${CMAKE_MATCH_1}" STREQUAL "${CKAS_ANGELSCRIPT_VERSION}")
        message(FATAL_ERROR
                "Expected AngelScript ${CKAS_ANGELSCRIPT_VERSION}, found '${CMAKE_MATCH_1}' in ${header}.\n"
                "Restore the matching SDK or move the old SDK aside and configure in a fresh build directory.")
    endif ()
endfunction()

function(ckas_prepare_angelscript)
    if (EXISTS "${CKAS_ANGELSCRIPT_SENTINEL}")
        ckas_check_angelscript_version("${CKAS_ANGELSCRIPT_SENTINEL}")
        return()
    endif ()

    if (EXISTS "${CKAS_ANGELSCRIPT_ROOT}")
        message(FATAL_ERROR
                "AngelScript source root exists but is incomplete: ${CKAS_ANGELSCRIPT_ROOT}\n"
                "Expected: ${CKAS_ANGELSCRIPT_SENTINEL}\n"
                "Remove the incomplete directory so CMake can download it, or populate it manually.")
    endif ()

    file(MAKE_DIRECTORY "${CKAS_ANGELSCRIPT_DOWNLOAD_DIR}")
    set(archive "${CKAS_ANGELSCRIPT_ARCHIVE}")
    if (archive STREQUAL "")
        set(download_url "${CKAS_ANGELSCRIPT_URL}")
        if (download_url STREQUAL "")
            set(download_url "https://www.angelcode.com/angelscript/sdk/files/angelscript_${CKAS_ANGELSCRIPT_VERSION}.zip")
        endif ()
        set(archive "${CKAS_ANGELSCRIPT_DOWNLOAD_DIR}/angelscript_${CKAS_ANGELSCRIPT_VERSION}.zip")
        message(STATUS "deps/angelscript is missing; downloading AngelScript SDK ${CKAS_ANGELSCRIPT_VERSION}")
        file(DOWNLOAD
                "${download_url}"
                "${archive}"
                EXPECTED_HASH SHA256=${CKAS_ANGELSCRIPT_SHA256}
                TLS_VERIFY ${CKAS_ANGELSCRIPT_TLS_VERIFY}
                SHOW_PROGRESS
                STATUS download_status
                LOG download_log
                HTTPHEADER "User-Agent: CMake/${CMAKE_VERSION}")
        list(GET download_status 0 download_code)
        if (NOT download_code EQUAL 0)
            file(REMOVE "${archive}")
            list(GET download_status 1 download_message)
            message(FATAL_ERROR
                    "Failed to download AngelScript SDK from ${download_url}: "
                    "${download_message}\n${download_log}")
        endif ()
    elseif (NOT EXISTS "${archive}")
        message(FATAL_ERROR "CKAS_ANGELSCRIPT_ARCHIVE does not exist: ${archive}")
    else ()
        file(SHA256 "${archive}" archive_sha256)
        if (NOT "${archive_sha256}" STREQUAL "${CKAS_ANGELSCRIPT_SHA256}")
            message(FATAL_ERROR
                    "CKAS_ANGELSCRIPT_ARCHIVE has unexpected SHA256.\n"
                    "Expected: ${CKAS_ANGELSCRIPT_SHA256}\n"
                    "Actual:   ${archive_sha256}\n"
                    "Archive:  ${archive}")
        endif ()
    endif ()

    set(extract_dir "${CKAS_ANGELSCRIPT_DOWNLOAD_DIR}/angelscript_${CKAS_ANGELSCRIPT_VERSION}")
    set(staged_root "${CKAS_ANGELSCRIPT_ROOT}.tmp")
    file(REMOVE_RECURSE "${extract_dir}" "${staged_root}")
    file(MAKE_DIRECTORY "${extract_dir}")
    execute_process(
            COMMAND "${CMAKE_COMMAND}" -E tar xf "${archive}"
            WORKING_DIRECTORY "${extract_dir}"
            RESULT_VARIABLE extract_result)
    if (NOT extract_result EQUAL 0)
        message(FATAL_ERROR "Failed to extract AngelScript SDK archive: ${archive}")
    endif ()

    if (NOT EXISTS "${extract_dir}/sdk/angelscript/include/angelscript.h")
        message(FATAL_ERROR "Downloaded AngelScript SDK archive has an unexpected layout.")
    endif ()

    ckas_check_angelscript_version("${extract_dir}/sdk/angelscript/include/angelscript.h")
    file(RENAME "${extract_dir}/sdk" "${staged_root}")
    file(RENAME "${staged_root}" "${CKAS_ANGELSCRIPT_ROOT}")
endfunction()

ckas_prepare_angelscript()

# CKAngelScript installs its own headers. The official SDK ZIP also omits
# the files needed by the upstream CMake package install rules.
set(AS_DISABLE_INSTALL ON CACHE INTERNAL "Disable installation of the embedded AngelScript library")
add_subdirectory("${CKAS_ANGELSCRIPT_ROOT}/angelscript/projects/cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/angelscript/angelscript/projects/cmake" EXCLUDE_FROM_ALL)
target_include_directories(angelscript INTERFACE
        $<BUILD_INTERFACE:${CKAS_ANGELSCRIPT_ROOT}/angelscript/include>)

add_library(angelscript_addon STATIC
        "${CKAS_ANGELSCRIPT_ROOT}/add_on/scriptany/scriptany.cpp"
        "${CKAS_ANGELSCRIPT_ROOT}/add_on/scriptarray/scriptarray.cpp"
        "${CKAS_ANGELSCRIPT_ROOT}/add_on/scriptstdstring/scriptstdstring.cpp"
        "${CKAS_ANGELSCRIPT_ROOT}/add_on/scriptstdstring/scriptstdstring_utils.cpp"
        "${CKAS_ANGELSCRIPT_ROOT}/add_on/scriptdictionary/scriptdictionary.cpp"
        "${CKAS_ANGELSCRIPT_ROOT}/add_on/scriptgrid/scriptgrid.cpp"
        "${CKAS_ANGELSCRIPT_ROOT}/add_on/scriptmath/scriptmath.cpp"
        "${CKAS_ANGELSCRIPT_ROOT}/add_on/scriptmath/scriptmathcomplex.cpp"
        "${CKAS_ANGELSCRIPT_ROOT}/add_on/scripthandle/scripthandle.cpp"
        "${CKAS_ANGELSCRIPT_ROOT}/add_on/weakref/weakref.cpp"
        "${CKAS_ANGELSCRIPT_ROOT}/add_on/datetime/datetime.cpp"
        "${CKAS_ANGELSCRIPT_ROOT}/add_on/scriptfile/scriptfile.cpp"
        "${CKAS_ANGELSCRIPT_ROOT}/add_on/scriptfile/scriptfilesystem.cpp"
        "${CKAS_ANGELSCRIPT_ROOT}/add_on/scripthelper/scripthelper.cpp"
        "${CKAS_ANGELSCRIPT_ROOT}/add_on/scriptbuilder/scriptbuilder.cpp"
        )
target_include_directories(angelscript_addon PUBLIC
        $<BUILD_INTERFACE:${CKAS_ANGELSCRIPT_ROOT}>)
target_link_libraries(angelscript_addon PUBLIC angelscript)

# fmt
add_subdirectory(fmt EXCLUDE_FROM_ALL)
