add_executable(CKAngelScriptCoreTests
        ScriptCoreTests.cpp
        ScriptInitialLifecycleSelfTest.cpp
        ScriptRuntimeMetadataSelfTest.cpp
        ScriptRuntimeDependencySelfTest.cpp
        "${PROJECT_SOURCE_DIR}/src/ScriptInitialLifecycle.cpp"
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
