set(CKAS_HEADERS
        ScriptManager.h
        ScriptFormat.h
        ScriptXString.h
        ScriptXBitArray.h
        ScriptVxMath.h
        ScriptCK2.h
)

set(CKAS_SOURCES
        Plugin.cpp
        ScriptManager.cpp
        AngelScriptLoader.cpp
        # AngelScriptRunner.cpp
        ScriptFormat.cpp
        ScriptXString.cpp
        ScriptXBitArray.cpp
        ScriptVxMath.cpp
        ScriptCK2.cpp
)

add_library(CKAngelScript SHARED ${CKAS_SOURCES} ${CKAS_HEADERS})
set_target_properties(CKAngelScript PROPERTIES OUTPUT_NAME "AngelScript")

target_include_directories(CKAngelScript
        PUBLIC
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(CKAngelScript PRIVATE
        CK2 VxMath
        angelscript angelscript_addon
        fmt::fmt
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