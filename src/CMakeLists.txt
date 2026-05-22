configure_file(Version.h.in "${CMAKE_CURRENT_SOURCE_DIR}/Version.h")

set(CKAS_HEADERS
        ScriptManager.h
        ScriptCache.h
        ScriptRunner.h
        ScriptInfo.h
        ScriptFormat.h
        ScriptNativePointer.h
        ScriptNativeBuffer.h
        ScriptDynCall.h
        ScriptXString.h
        ScriptXArray.h
        ScriptXHashTable.h
        ScriptXBitArray.h
        ScriptVxMath.h
        ScriptCKTypes.h
        ScriptCKEnums.h
        ScriptCKDefines.h
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
        ScriptAsync.h
        ScriptSelfTests.h
        ScriptRefCounted.h
        ScriptBehaviorBridge.h
        ScriptComponentMetadata.h
        ScriptComponentInjection.h
        ScriptComponentAutomation.h
        ScriptBridgeCommon.h
        ScriptBridgeHandles.h
        ScriptBridgeParameterIO.h
        ScriptBridgeSnapshot.h
)

set(CKAS_SOURCES
        Plugin.cpp
        ScriptManager.cpp
        ScriptCache.cpp
        ScriptRunner.cpp
        AngelScriptLoader.cpp
        AngelScriptRunner.cpp
        AngelScriptComponent.cpp
        ScriptComponentMetadata.cpp
        ScriptComponentInjection.cpp
        ScriptComponentAutomation.cpp
        ScriptInfo.cpp
        ScriptFormat.cpp
        ScriptNativePointer.cpp
        ScriptNativeBuffer.cpp
        ScriptDynCall.cpp
        ScriptXString.cpp
        ScriptXBitArray.cpp
        ScriptVxMath.cpp
        ScriptCKTypes.cpp
        ScriptCKEnums.cpp
        ScriptCKDefines.cpp
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
        ScriptAsync.cpp
        ScriptBehaviorBridge.cpp
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

        AngelScript.rc
)

if (CKAS_BUILD_SELF_TESTS)
    list(APPEND CKAS_SOURCES
            ScriptStartupSelfTest.cpp
            ScriptComponentMetadataSelfTest.cpp
            ScriptParameterConversionSelfTest.cpp
            ScriptParameterRegistrySelfTest.cpp
            ScriptRuntimeMetadataSelfTest.cpp
            ScriptRuntimeDependencySelfTest.cpp
            ScriptRuntimeSelfTest.cpp
            ScriptAsyncSelfTest.cpp
            ScriptBridgeSelfTest.cpp
    )
endif ()

add_library(CKAngelScript SHARED ${CKAS_SOURCES} ${CKAS_HEADERS})
set_target_properties(CKAngelScript PROPERTIES OUTPUT_NAME "AngelScript")

target_compile_definitions(CKAngelScript PRIVATE
        CKAS_BUILD_SELF_TESTS=$<BOOL:${CKAS_BUILD_SELF_TESTS}>
)

target_include_directories(CKAngelScript
        PUBLIC
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${DYNCALL_INCLUDE_DIR}
        ${DYNCALLBACK_INCLUDE_DIR}
        ${DYNLOAD_INCLUDE_DIR}
)

target_link_libraries(CKAngelScript PRIVATE
        CK2 VxMath
        angelscript angelscript_addon
        fmt::fmt
        ${DYNCALL_LIBRARY}
        ${DYNCALLBACK_LIBRARY}
        ${DYNLOAD_LIBRARY}
)

if (NOT SKIP_INSTALL_ALL)
    install(TARGETS CKAngelScript
            RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/BuildingBlocks
    )

    if (NOT SKIP_INSTALL_HEADERS)
        install(FILES
                ${PROJECT_SOURCE_DIR}/include/AngelScriptManager.h
                ${PROJECT_SOURCE_DIR}/deps/angelscript/angelscript/include/angelscript.h
                DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    endif ()
endif ()
