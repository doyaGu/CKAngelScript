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

if(CKAS_RUNTIME_SDL OR CKAS_ENGINE_TEST_RUNTIME_DIR)
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

    add_test(NAME CKAngelScript.Engine COMMAND CKAngelScriptEngineTests $<TARGET_FILE:CKAngelScript>)
    if(WIN32 AND CKAS_RUNTIME_SDL)
        set_property(TEST CKAngelScript.Engine PROPERTY ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:CK2>"
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:VxMath>"
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:SDL3::SDL3>")
    elseif(WIN32)
        set_property(TEST CKAngelScript.Engine PROPERTY ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:${CKAS_ENGINE_TEST_RUNTIME_DIR}")
        set_property(TEST CKAngelScript.Engine PROPERTY ENVIRONMENT
            "CKAS_DISABLE_CK2_PROTOTYPE_HOOK=1")
    endif()
endif()
