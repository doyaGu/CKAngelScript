#include "ScriptManager.h"

#include <string>
#include <utility>

#include <fmt/format.h>

asIScriptEngine *ScriptManager::GetScriptEngine() {
    return m_EngineHost.Engine();
}

void ScriptManager::MessageCallback(const asSMessageInfo &msg) {
    const char *type = "NULL";
    CKAS_MESSAGETYPE publicType = CKAS_MESSAGE_INFORMATION;
    switch (msg.type) {
        case asMSGTYPE_ERROR:
            type = "ERROR";
            publicType = CKAS_MESSAGE_ERROR;
            break;
        case asMSGTYPE_WARNING:
            type = "WARN";
            publicType = CKAS_MESSAGE_WARNING;
            break;
        case asMSGTYPE_INFORMATION:
            type = "INFO";
            publicType = CKAS_MESSAGE_INFORMATION;
            break;
    }
    const std::string formatted = fmt::format("{}({},{}): {}: {}",
                                              msg.section ? msg.section : "<unknown section>",
                                              msg.row,
                                              msg.col,
                                              type,
                                              msg.message ? msg.message : "");
    if (m_Diagnostics.IsCapturingScriptMessages()) {
        CapturedScriptMessage captured;
        captured.Section = msg.section ? msg.section : "";
        captured.Row = msg.row;
        captured.Column = msg.col;
        captured.Type = publicType;
        captured.Message = msg.message ? msg.message : "";
        m_Diagnostics.CaptureScriptMessage(formatted, std::move(captured));
    }
    m_Context->OutputToConsoleEx(const_cast<char *>("%s"), formatted.c_str());
}

void ScriptManager::ExceptionCallback(asIScriptContext *context) {
    std::string callStack = GetCallStack(context);
    asIScriptFunction *func = context ? context->GetExceptionFunction() : nullptr;
    const char *funcDecl = func ? func->GetDeclaration() : nullptr;
    const char *exception = context ? context->GetExceptionString() : nullptr;
    std::string message = fmt::format("Exception in '{}': '{}'\n{}",
                                      funcDecl ? funcDecl : "<unknown function>",
                                      exception ? exception : "",
                                      callStack);

    asSMessageInfo info = {};
    if (context) {
        info.row = context->GetExceptionLineNumber(&info.col, &info.section);
    }
    info.type = asMSGTYPE_ERROR;
    info.message = message.c_str();
    MessageCallback(info);
}

std::string ScriptManager::GetCallStack(asIScriptContext *context) {
    std::string str("Callstack:\n");
    if (!context) {
        return str;
    }

    for (asUINT i = 0; i < context->GetCallstackSize(); i++) {
        asIScriptFunction *func = context->GetFunction(i);
        int column;
        const char *section;
        int line = context->GetLineNumber(i, &column, &section);
        const char *funcDecl = func ? func->GetDeclaration() : nullptr;
        str.append(fmt::format("\t{} at {}({},{})\n",
                               funcDecl ? funcDecl : "<unknown function>",
                               section ? section : "<unknown section>",
                               line,
                               column));
    }
    return std::move(str);
}

asIScriptContext *ScriptManager::RequestContextFromPool() {
    return m_EngineHost.RequestContext(*this);
}

void ScriptManager::ReturnContextToPool(asIScriptContext *ctx) {
    m_EngineHost.ReturnContext(*this, ctx);
}
