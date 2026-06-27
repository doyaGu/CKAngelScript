#include "ScriptSelfTests.h"

#include <cstdio>
#include <cstring>
#include <string>

#include <fmt/format.h>

#include "CKAngelScript.h"
#include "ScriptManager.h"

namespace {

bool ExecuteNativeMemoryFunction(asIScriptEngine *engine,
                                 asIScriptFunction *function,
                                 const char *label,
                                 bool expectException,
                                 const char *expectedException,
                                 std::string &error) {
    if (!engine || !function) {
        error = fmt::format("Native memory self-test function '{}' is unavailable.", label ? label : "<unknown>");
        return false;
    }

    asIScriptContext *context = engine->RequestContext();
    if (!context) {
        error = "Native memory self-test could not create AngelScript execution context.";
        return false;
    }

    int r = context->Prepare(function);
    if (r >= 0) {
        r = context->Execute();
    }

    bool ok = false;
    if (expectException) {
        if (r == asEXECUTION_EXCEPTION) {
            const char *exception = context->GetExceptionString();
            if (!expectedException || !expectedException[0] ||
                (exception && std::strstr(exception, expectedException))) {
                ok = true;
            } else {
                error = fmt::format("Native memory self-test '{}' threw '{}', expected '{}'.",
                                    label,
                                    exception && exception[0] ? exception : "<empty>",
                                    expectedException);
            }
        } else {
            error = fmt::format("Native memory self-test '{}' finished without expected exception ({}).",
                                label,
                                r);
        }
    } else if (r == asEXECUTION_FINISHED) {
        const int returnCode = static_cast<int>(context->GetReturnDWord());
        if (returnCode == 0) {
            ok = true;
        } else {
            error = fmt::format("Native memory self-test '{}' returned {}.", label, returnCode);
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = context->GetExceptionString();
        error = fmt::format("Native memory self-test '{}' exception: {}.",
                            label,
                            exception && exception[0] ? exception : "<empty>");
    } else {
        error = fmt::format("Native memory self-test '{}' execution failed ({}).", label, r);
    }

    const int state = context->GetState();
    if (state == asEXECUTION_ACTIVE ||
        state == asEXECUTION_SUSPENDED ||
        state == asEXECUTION_PREPARED ||
        state == asEXECUTION_EXCEPTION ||
        state == asEXECUTION_ABORTED) {
        context->Abort();
    }
    context->Unprepare();
    engine->ReturnContext(context);
    return ok;
}

} // namespace

bool RunScriptNativeMemorySelfTest(CKContext *context, asIScriptEngine *engine, std::string &error) {
    if (!context || !engine) {
        error = "Native memory self-test requires a CKContext and AngelScript engine.";
        return false;
    }

    ScriptManager *manager = ScriptManager::GetManager(context);
    if (!manager) {
        error = "Native memory self-test could not retrieve CKAngelScript manager.";
        return false;
    }

    asITypeInfo *nativePointerType = engine->GetTypeInfoByDecl("NativePointer");
    asITypeInfo *nativeBufferType = engine->GetTypeInfoByDecl("NativeBuffer");
    if (!nativePointerType || !nativeBufferType) {
        error = "Native memory self-test could not find NativePointer or NativeBuffer type.";
        return false;
    }
    if (nativePointerType->GetMethodByDecl("size_t Write(?&in)") == nullptr ||
        nativePointerType->GetMethodByDecl("size_t Read(?&out)") == nullptr ||
        nativeBufferType->GetMethodByDecl("size_t Write(?&in)") == nullptr ||
        nativeBufferType->GetMethodByDecl("size_t Read(?&out)") == nullptr) {
        error = "Native memory self-test could not find expected size_t generic read/write methods.";
        return false;
    }
    constexpr const char *moduleName = "__CKAS_NativeMemorySelfTest";
    const char *source =
        "class NativeMemoryRejectBox { int value; }\n"
        "NativePointer MakePointerFromLocalBuffer() {\n"
        "  NativeBuffer@ buffer = NativeBuffer(4);\n"
        "  buffer.WriteUInt(0x13572468);\n"
        "  buffer.Seek(0);\n"
        "  return buffer.ToPointer();\n"
        "}\n"
        "NativeBuffer@ MakeBufferFromLocalPointer() {\n"
        "  return NativeBuffer(MakePointerFromLocalBuffer(), 4);\n"
        "}\n"
        "NativePointer MakePointerFromLocalArray() {\n"
        "  array<int> values = { 91, 92, 93 };\n"
        "  return NativePointer(values);\n"
        "}\n"
        "int Run() {\n"
        "  NativeBuffer@ buffer = NativeBuffer(256);\n"
        "  if (buffer is null || !buffer.IsValid()) return 1;\n"
        "  int writtenInt = 0x12345678;\n"
        "  if (buffer.Write(writtenInt) != 4) return 2;\n"
        "  if (!buffer.Seek(0)) return 3;\n"
        "  int readInt = 0;\n"
        "  if (buffer.Read(readInt) != 4 || readInt != writtenInt) return 4;\n"
        "  if (!buffer.Seek(0)) return 5;\n"
        "  double writtenDouble = 42.5;\n"
        "  if (buffer.Write(writtenDouble) != 8) return 6;\n"
        "  if (!buffer.Seek(0)) return 7;\n"
        "  double readDouble = 0.0;\n"
        "  if (buffer.Read(readDouble) != 8 || readDouble != writtenDouble) return 8;\n"
        "  if (!buffer.Seek(0)) return 9;\n"
        "  VxVector writtenVector(1.25f, 2.5f, 3.75f);\n"
        "  size_t vectorSize = buffer.Write(writtenVector);\n"
        "  if (vectorSize == 0) return 10;\n"
        "  if (!buffer.Seek(0)) return 11;\n"
        "  VxVector readVector;\n"
        "  if (buffer.Read(readVector) != vectorSize) return 12;\n"
        "  if (readVector.x != writtenVector.x || readVector.y != writtenVector.y || readVector.z != writtenVector.z) return 13;\n"
        "  if (!buffer.Seek(0)) return 14;\n"
        "  string writtenString = \"native-memory\";\n"
        "  size_t writtenStringBytes = buffer.Write(writtenString);\n"
        "  if (writtenStringBytes != 14) return 15;\n"
        "  if (!buffer.Seek(0)) return 16;\n"
        "  string readString;\n"
        "  if (buffer.Read(readString) != writtenStringBytes || readString != writtenString) return 17;\n"
        "  if (!buffer.Seek(0)) return 18;\n"
        "  NativePointer pointer = buffer.ToPointer();\n"
        "  if (pointer.IsNull()) return 19;\n"
        "  uint writtenUInt = 0x00ABCDEF;\n"
        "  if (pointer.Write(writtenUInt) != 4) return 20;\n"
        "  uint readUInt = 0;\n"
        "  if (pointer.Read(readUInt) != 4 || readUInt != writtenUInt) return 21;\n"
        "  if (pointer.Write(writtenString) == 0) return 22;\n"
        "  string pointerString;\n"
        "  if (pointer.Read(pointerString) == 0 || pointerString != writtenString) return 23;\n"
        "  array<int> values = { 7, 8, 9 };\n"
        "  NativePointer arrayPointer(values);\n"
        "  if (arrayPointer.IsNull()) return 24;\n"
        "  int firstValue = 0;\n"
        "  if (arrayPointer.Read(firstValue) != 4 || firstValue != 7) return 25;\n"
        "  NativePointer nextPointer = pointer + 4;\n"
        "  if (!(pointer < nextPointer)) return 26;\n"
        "  if (nextPointer < pointer) return 27;\n"
        "  if (!(nextPointer > pointer)) return 28;\n"
        "  if (pointer > nextPointer) return 29;\n"
        "  if (!(pointer <= pointer) || !(pointer >= pointer)) return 30;\n"
        "  if (!buffer.Seek(0)) return 31;\n"
        "  uint original = 0x11111111;\n"
        "  uint replacement = 0x22222222;\n"
        "  if (buffer.Write(original) != 4) return 32;\n"
        "  if (!buffer.Seek(0)) return 33;\n"
        "  NativeBuffer@ extracted = buffer.Extract(4);\n"
        "  if (extracted is null || !extracted.IsValid() || extracted.Size() != 4) return 34;\n"
        "  if (!buffer.Seek(0)) return 35;\n"
        "  if (buffer.Write(replacement) != 4) return 36;\n"
        "  uint extractedValue = 0;\n"
        "  if (extracted.Read(extractedValue) != 4 || extractedValue != original) return 37;\n"
        "  if (!buffer.Seek(0)) return 38;\n"
        "  uint8 indexedWrite = 242;\n"
        "  buffer[0] = indexedWrite;\n"
        "  uint8 indexedRead = buffer[0];\n"
        "  if (indexedRead != indexedWrite) return 39;\n"
        "  const NativeBuffer@ constBuffer = buffer;\n"
        "  if (constBuffer[0] != indexedWrite) return 40;\n"
        "  NativePointer retainedBufferPointer = MakePointerFromLocalBuffer();\n"
        "  uint retainedBufferValue = 0;\n"
        "  if (retainedBufferPointer.Read(retainedBufferValue) != 4 || retainedBufferValue != 0x13572468) return 41;\n"
        "  NativePointer retainedArrayPointer = MakePointerFromLocalArray();\n"
        "  int retainedArrayValue = 0;\n"
        "  if (retainedArrayPointer.Read(retainedArrayValue) != 4 || retainedArrayValue != 91) return 42;\n"
        "  NativeBuffer@ retainedWrappedBuffer = MakeBufferFromLocalPointer();\n"
        "  uint retainedWrappedValue = 0;\n"
        "  if (retainedWrappedBuffer is null || retainedWrappedBuffer.Read(retainedWrappedValue) != 4 || retainedWrappedValue != 0x13572468) return 43;\n"
        "  NativePointer retainedArraySecondPointer = retainedArrayPointer + 4;\n"
        "  int retainedArraySecondValue = 0;\n"
        "  if (retainedArraySecondPointer.Read(retainedArraySecondValue) != 4 || retainedArraySecondValue != 92) return 44;\n"
        "  NativeBuffer@ endBuffer = NativeBuffer(4);\n"
        "  if (!endBuffer.Seek(endBuffer.Size()) || !endBuffer.ToPointer().IsNull()) return 45;\n"
        "  NativeBuffer@ smallBuffer = NativeBuffer(1);\n"
        "  uint tooLarge = 0x99999999;\n"
        "  if (smallBuffer.Write(tooLarge) != 0 || smallBuffer.CursorPos() != 0) return 46;\n"
        "  if (smallBuffer.Read(tooLarge) != 0 || smallBuffer.CursorPos() != 0) return 47;\n"
        "  string fileName = \"ckas-native-memory-selftest.bin\";\n"
        "  NativeBuffer@ fileBuffer = NativeBuffer(4);\n"
        "  if (fileBuffer.WriteUInt(0x2468ACE0) != 4 || !fileBuffer.Seek(0)) return 48;\n"
        "  if (fileBuffer.Save(fileName, 4) != 4) return 49;\n"
        "  NativeBuffer@ loadedBuffer = NativeBuffer(4);\n"
        "  if (loadedBuffer.Load(fileName, 4) != 4 || !loadedBuffer.Seek(0)) return 50;\n"
        "  uint loadedValue = 0;\n"
        "  if (loadedBuffer.ReadUInt(loadedValue) != 4 || loadedValue != 0x2468ACE0) return 51;\n"
        "  if (!buffer.Seek(0)) return 52;\n"
        "  if (buffer.WriteString(\"abc\") != 4) return 53;\n"
        "  if (!buffer.Seek(0)) return 54;\n"
        "  string explicitString;\n"
        "  if (buffer.ReadString(explicitString) != 4 || explicitString != \"abc\") return 55;\n"
        "  if (buffer.CursorPos() != 4) return 56;\n"
        "  if (!buffer.Seek(0)) return 57;\n"
        "  NativePointer emptyPointer = buffer.ToPointer();\n"
        "  if (emptyPointer.WriteString(\"\") != 1) return 58;\n"
        "  string emptyPointerString = \"not-empty\";\n"
        "  if (emptyPointer.ReadString(emptyPointerString) != 1 || emptyPointerString != \"\") return 59;\n"
        "  return 0;\n"
        "}\n"
        "void RejectBufferWriteObject() { NativeBuffer@ buffer = NativeBuffer(16); NativeMemoryRejectBox box; buffer.Write(box); }\n"
        "void RejectBufferWriteHandle() { NativeBuffer@ buffer = NativeBuffer(16); NativeBuffer@ other = NativeBuffer(1); buffer.Write(@other); }\n"
        "void RejectBufferReadObject() { NativeBuffer@ buffer = NativeBuffer(16); NativeMemoryRejectBox box; buffer.Read(box); }\n"
        "void RejectBufferReadHandle() { NativeBuffer@ buffer = NativeBuffer(16); NativeBuffer@ other; buffer.Read(@other); }\n"
        "void RejectPointerConstructObject() { NativeMemoryRejectBox box; NativePointer pointer(box); }\n"
        "void RejectPointerConstructHandle() { NativeBuffer@ buffer = NativeBuffer(16); NativePointer pointer(@buffer); }\n"
        "void RejectPointerWriteObject() { NativeBuffer@ buffer = NativeBuffer(16); NativePointer pointer = buffer.ToPointer(); NativeMemoryRejectBox box; pointer.Write(box); }\n"
        "void RejectPointerWriteHandle() { NativeBuffer@ buffer = NativeBuffer(16); NativePointer pointer = buffer.ToPointer(); NativeBuffer@ other = NativeBuffer(1); pointer.Write(@other); }\n"
        "void RejectPointerReadObject() { NativeBuffer@ buffer = NativeBuffer(16); NativePointer pointer = buffer.ToPointer(); NativeMemoryRejectBox box; pointer.Read(box); }\n"
        "void RejectPointerReadHandle() { NativeBuffer@ buffer = NativeBuffer(16); NativePointer pointer = buffer.ToPointer(); NativeBuffer@ other; pointer.Read(@other); }\n";

    CKAngelScriptResult compileResult = {};
    if (manager->CompileModule(moduleName, source, CKAS_COMPILE_REPLACEEXISTING, &compileResult) != CKAS_OK) {
        error = compileResult.ErrorMessage && compileResult.ErrorMessage[0] != '\0'
            ? compileResult.ErrorMessage
            : "Native memory self-test compile failed.";
        return false;
    }

    asIScriptModule *module = manager->GetModule(moduleName);
    if (!module) {
        manager->UnloadModule(moduleName, nullptr);
        error = "Native memory self-test could not retrieve compiled module.";
        return false;
    }

    struct ExpectedException {
        const char *Decl;
        const char *Message;
    };

    const ExpectedException expectedExceptions[] = {
        {"void RejectBufferWriteObject()", "Cannot write script objects to buffer"},
        {"void RejectBufferWriteHandle()", "Cannot write object handle to buffer"},
        {"void RejectBufferReadObject()", "Cannot read script objects from buffer"},
        {"void RejectBufferReadHandle()", "Cannot read object handle from buffer"},
        {"void RejectPointerConstructObject()", "Cannot create NativePointer from script objects or object handles"},
        {"void RejectPointerConstructHandle()", "Cannot create NativePointer from script objects or object handles"},
        {"void RejectPointerWriteObject()", "Cannot write script objects to buffer"},
        {"void RejectPointerWriteHandle()", "Cannot write object handle to buffer"},
        {"void RejectPointerReadObject()", "Cannot read script objects from buffer"},
        {"void RejectPointerReadHandle()", "Cannot read object handle from buffer"},
    };

    bool ok = ExecuteNativeMemoryFunction(engine,
                                          module->GetFunctionByDecl("int Run()"),
                                          "Run",
                                          false,
                                          nullptr,
                                          error);

    for (const ExpectedException &expected : expectedExceptions) {
        if (!ok) {
            break;
        }
        ok = ExecuteNativeMemoryFunction(engine,
                                         module->GetFunctionByDecl(expected.Decl),
                                         expected.Decl,
                                         true,
                                         expected.Message,
                                         error);
    }

    manager->UnloadModule(moduleName, nullptr);
    engine->GarbageCollect(asGC_ONE_STEP | asGC_DETECT_GARBAGE | asGC_DESTROY_GARBAGE);
    std::remove("ckas-native-memory-selftest.bin");
    return ok;
}
