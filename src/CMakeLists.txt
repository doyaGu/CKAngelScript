set(CKAS_GENERATED_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
configure_file(Version.h.in "${CKAS_GENERATED_INCLUDE_DIR}/Version.h")

set(CKAS_HEADERS
        Logger.h
        ScriptManager.h
        AngelScriptEventHook.h
        ScriptApiDiagnostics.h
        ScriptApiHandles.h
        ScriptCache.h
        ScriptInvoker.h
        ScriptFormat.h
        ScriptNativePointer.h
        ScriptNativeBuffer.h
        ScriptXString.h
        ScriptXArray.h
        ScriptXHashTable.h
        ScriptXBitArray.h
        ScriptVxMath.h
        ScriptCKTypes.h
        ScriptCKEnums.h
        ScriptCKDefines.h
        ScriptCKVertexBuffer.h
        ScriptXObjectArray.h
        ScriptCKContext.h
        ScriptCKManagers.h
        ScriptCKObjects.h
        ScriptCK2.h
        ScriptParameterConversion.h
        ScriptParameterRegistry.h
        ScriptRuntime.h
        ScriptRuntimeMetadata.h
        ScriptRuntimeDependency.h
        ScriptMessage.h
        ScriptObjectRef.h
        ScriptScene.h
        ScriptAsync.h
        ScriptRefCounted.h
        ScriptBehaviorBridge.h
        ScriptCKObjectRetainer.h
        ScriptComponentMetadata.h
        ScriptComponentState.h
        ScriptComponentInjection.h
        ScriptComponentAutomation.h
        ScriptBridgeCommon.h
        ScriptBridgeHandles.h
        ScriptBridgeParameterIO.h
        ScriptBridgeSnapshot.h
        ScriptRegistration.h
        ScriptPublicApiInternal.h
)

set(CKAS_SOURCES
        Plugin.cpp
        ScriptPublicApi.cpp
        Logger.cpp
        ScriptManager.cpp
        ScriptApiDiagnostics.cpp
        ScriptModuleRegistry.cpp
        ScriptCache.cpp
        ScriptInvoker.cpp
        AngelScriptEventHook.cpp
        AngelScriptComponent.cpp
        ScriptComponentMetadata.cpp
        ScriptComponentInjection.cpp
        ScriptComponentAutomation.cpp
        ScriptFormat.cpp
        ScriptNativePointer.cpp
        ScriptNativeBuffer.cpp
        ScriptXString.cpp
        ScriptXBitArray.cpp
        ScriptVxMath.cpp
        ScriptCKTypes.cpp
        ScriptCKEnums.cpp
        ScriptCKDefines.cpp
        ScriptCKVertexBuffer.cpp
        ScriptXObjectArray.cpp
        ScriptCKContext.cpp
        ScriptCKManagers.cpp
        ScriptCKObjects.cpp
        ScriptCK2.cpp
        ScriptParameterConversion.cpp
        ScriptParameterRegistry.cpp
        ScriptRuntime.cpp
        ScriptRuntimeMetadata.cpp
        ScriptRuntimeDependency.cpp
        ScriptMessage.cpp
        ScriptObjectRef.cpp
        ScriptScene.cpp
        ScriptAsync.cpp
        ScriptBehaviorBridge.cpp
        ScriptCKObjectRetainer.cpp
        ScriptBridgeRuntime.cpp
        ScriptBridgeLayoutCache.cpp
        ScriptBridgeWatch.cpp
        ScriptBridgeCommon.cpp
        ScriptBridgeParameterIO.cpp
        ScriptBridgeRegistration.cpp
        ScriptBridgeParams.cpp
        ScriptBridgeBehavior.cpp
        ScriptBridgeGraph.cpp
        ScriptBridgeBB.cpp
        ScriptBridgeTasks.cpp
        ScriptRegistration.cpp

        AngelScript.rc
)

if (CKAS_ENABLE_DYNCALL)
    list(APPEND CKAS_HEADERS
            ScriptDynCall.h
    )

    list(APPEND CKAS_SOURCES
            ScriptDynCall.cpp
    )
endif ()

if (CKAS_BUILD_SELF_TESTS)
    set(CKAS_TEST_DIR "${PROJECT_SOURCE_DIR}/tests")
    list(APPEND CKAS_SOURCES
            "${CKAS_TEST_DIR}/ScriptStartupSelfTest.cpp"
            "${CKAS_TEST_DIR}/ScriptComponentMetadataSelfTest.cpp"
            "${CKAS_TEST_DIR}/ScriptApiSelfTest.cpp"
            "${CKAS_TEST_DIR}/ScriptInfoExportSelfTest.cpp"
            "${CKAS_TEST_DIR}/ScriptParameterConversionSelfTest.cpp"
            "${CKAS_TEST_DIR}/ScriptParameterRegistrySelfTest.cpp"
            "${CKAS_TEST_DIR}/ScriptRuntimeMetadataSelfTest.cpp"
            "${CKAS_TEST_DIR}/ScriptRuntimeDependencySelfTest.cpp"
            "${CKAS_TEST_DIR}/ScriptNativeMemorySelfTest.cpp"
            "${CKAS_TEST_DIR}/ScriptMessageSelfTest.cpp"
            "${CKAS_TEST_DIR}/ScriptSceneSelfTest.cpp"
            "${CKAS_TEST_DIR}/ScriptRuntimeSelfTest.cpp"
            "${CKAS_TEST_DIR}/ScriptAsyncSelfTest.cpp"
            "${CKAS_TEST_DIR}/ScriptBridgeSelfTest.cpp"
            "${CKAS_TEST_DIR}/ScriptVxBindingSelfTest.cpp"
    )
    if (CKAS_ENABLE_DYNCALL)
        list(APPEND CKAS_SOURCES
                "${CKAS_TEST_DIR}/ScriptDynLoadSelfTest.cpp"
        )
    endif ()
endif ()

if (CKAS_ENABLE_API_EXPORT)
    list(APPEND CKAS_HEADERS
            ScriptInfo.h
    )

    list(APPEND CKAS_SOURCES
            ScriptInfo.cpp
    )
endif ()

add_library(CKAngelScript SHARED ${CKAS_SOURCES} ${CKAS_HEADERS})
set_target_properties(CKAngelScript PROPERTIES OUTPUT_NAME "AngelScript")

target_compile_definitions(CKAngelScript PRIVATE
        CKAS_BUILDING_DLL
        CKAS_BUILD_SELF_TESTS=$<BOOL:${CKAS_BUILD_SELF_TESTS}>
        CKAS_ENABLE_DYNCALL=$<BOOL:${CKAS_ENABLE_DYNCALL}>
        CKAS_ENABLE_API_EXPORT=$<BOOL:${CKAS_ENABLE_API_EXPORT}>
)

target_include_directories(CKAngelScript
        PUBLIC
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CKAS_GENERATED_INCLUDE_DIR}
)

if (CKAS_BUILD_SELF_TESTS)
    target_include_directories(CKAngelScript PRIVATE
            "${PROJECT_SOURCE_DIR}/tests"
    )
endif ()

target_link_libraries(CKAngelScript PRIVATE
        CK2 VxMath
        angelscript angelscript_addon
        fmt::fmt
)

if (CKAS_ENABLE_DYNCALL)
    target_include_directories(CKAngelScript PRIVATE
            ${DYNCALL_INCLUDE_DIR}
            ${DYNCALLBACK_INCLUDE_DIR}
            ${DYNLOAD_INCLUDE_DIR}
    )

    target_link_libraries(CKAngelScript PRIVATE
            ${DYNCALL_LIBRARY}
            ${DYNCALLBACK_LIBRARY}
            ${DYNLOAD_LIBRARY}
    )
endif ()

if (NOT SKIP_INSTALL_ALL)
    install(TARGETS CKAngelScript
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    if (NOT SKIP_INSTALL_HEADERS)
        install(FILES
                ${PROJECT_SOURCE_DIR}/include/CKAngelScript.h
                ${CKAS_ANGELSCRIPT_ROOT}/angelscript/include/angelscript.h
                DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    endif ()
endif ()
