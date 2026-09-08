add_executable(CKAngelScriptCoreTests
        ScriptCoreTests.cpp
        ScriptInitialLifecycleSelfTest.cpp
        ScriptModuleGenerationSelfTest.cpp
        ScriptRuntimeMetadataSelfTest.cpp
        ScriptRuntimeDependencySelfTest.cpp
        "${PROJECT_SOURCE_DIR}/src/ScriptInitialLifecycle.cpp"
        "${PROJECT_SOURCE_DIR}/src/ScriptModuleGeneration.cpp"
        "${PROJECT_SOURCE_DIR}/src/ScriptRuntimeMetadata.cpp"
        "${PROJECT_SOURCE_DIR}/src/ScriptRuntimeDependency.cpp"
)

target_include_directories(CKAngelScriptCoreTests PRIVATE
        "${PROJECT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(CKAngelScriptCoreTests PRIVATE fmt::fmt)

set_target_properties(CKAngelScriptCoreTests PROPERTIES
        FOLDER "Tests"
)

add_test(NAME CKAngelScript.Core COMMAND CKAngelScriptCoreTests)

if (CKAS_ENGINE_TEST_RUNTIME_DIR)
    foreach (_ckas_runtime_dll CK2.dll VxMath.dll)
        if (NOT EXISTS "${CKAS_ENGINE_TEST_RUNTIME_DIR}/${_ckas_runtime_dll}")
            message(FATAL_ERROR
                    "CKAS_ENGINE_TEST_RUNTIME_DIR is missing ${_ckas_runtime_dll}: "
                    "${CKAS_ENGINE_TEST_RUNTIME_DIR}")
        endif ()
    endforeach ()

    add_executable(CKAngelScriptEngineTests
            ScriptEngineIntegrationTest.cpp
            "${PROJECT_SOURCE_DIR}/src/ScriptObjectIdentity.cpp"
    )
    target_include_directories(CKAngelScriptEngineTests PRIVATE
            "${PROJECT_SOURCE_DIR}/include"
            "${PROJECT_SOURCE_DIR}/src"
            "${CKAS_ANGELSCRIPT_ROOT}/angelscript/include"
    )
    target_link_libraries(CKAngelScriptEngineTests PRIVATE CK2 VxMath)
    add_dependencies(CKAngelScriptEngineTests CKAngelScript)
    set_target_properties(CKAngelScriptEngineTests PROPERTIES FOLDER "Tests")

    add_test(NAME CKAngelScript.Engine
            COMMAND ${CMAKE_COMMAND} -E env
            "PATH=${CKAS_ENGINE_TEST_RUNTIME_DIR};$ENV{PATH}"
            "CKAS_DISABLE_CK2_PROTOTYPE_HOOK=1"
            $<TARGET_FILE:CKAngelScriptEngineTests>
            $<TARGET_FILE:CKAngelScript>)
endif ()
