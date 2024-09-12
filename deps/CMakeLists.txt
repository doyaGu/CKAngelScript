# AngelScript
add_subdirectory(angelscript/angelscript/projects/cmake EXCLUDE_FROM_ALL)
target_include_directories(angelscript INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/angelscript/angelscript/include>)

add_library(angelscript_addon STATIC
        angelscript/add_on/scriptany/scriptany.cpp
        angelscript/add_on/scriptarray/scriptarray.cpp
        angelscript/add_on/scriptstdstring/scriptstdstring.cpp
        angelscript/add_on/scriptstdstring/scriptstdstring_utils.cpp
        angelscript/add_on/scriptdictionary/scriptdictionary.cpp
        angelscript/add_on/scriptmath/scriptmath.cpp
        angelscript/add_on/scriptmath/scriptmathcomplex.cpp
        angelscript/add_on/scripthandle/scripthandle.cpp
        angelscript/add_on/weakref/weakref.cpp
        angelscript/add_on/datetime/datetime.cpp
        angelscript/add_on/scriptfile/scriptfile.cpp
        angelscript/add_on/scriptfile/scriptfilesystem.cpp
        angelscript/add_on/scripthelper/scripthelper.cpp
        angelscript/add_on/scriptbuilder/scriptbuilder.cpp
        )
target_include_directories(angelscript_addon PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/angelscript>)
target_link_libraries(angelscript_addon PUBLIC angelscript)

# fmt
add_subdirectory(fmt EXCLUDE_FROM_ALL)