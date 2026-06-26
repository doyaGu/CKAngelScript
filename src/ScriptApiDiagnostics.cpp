#include "ScriptApiDiagnostics.h"

#include <utility>

CKAngelScriptResult ScriptApiDiagnostics::MakeResult(
    CKAS_STATUS status,
    int angelScriptCode,
    const std::string &errorMessage,
    const std::string &stackTrace,
    const std::vector<CapturedScriptMessage> *compilerMessages) {
    m_LastErrorMessage = errorMessage;
    m_LastStackTrace = stackTrace;
    m_LastCompilerMessageStorage.clear();
    m_LastCompilerMessages.clear();
    if (compilerMessages && !compilerMessages->empty()) {
        m_LastCompilerMessageStorage = *compilerMessages;
        m_LastCompilerMessages.reserve(m_LastCompilerMessageStorage.size());
        for (const CapturedScriptMessage &message : m_LastCompilerMessageStorage) {
            CKAngelScriptCompilerMessage publicMessage = {};
            publicMessage.Size = sizeof(publicMessage);
            publicMessage.Section = message.Section.empty() ? nullptr : message.Section.c_str();
            publicMessage.Row = message.Row;
            publicMessage.Column = message.Column;
            publicMessage.Type = message.Type;
            publicMessage.Message = message.Message.empty() ? nullptr : message.Message.c_str();
            m_LastCompilerMessages.push_back(publicMessage);
        }
    }

    CKAngelScriptResult result;
    result.Size = sizeof(result);
    result.Status = status;
    result.AngelScriptCode = angelScriptCode;
    result.ErrorMessage = m_LastErrorMessage.empty() ? nullptr : m_LastErrorMessage.c_str();
    result.StackTrace = m_LastStackTrace.empty() ? nullptr : m_LastStackTrace.c_str();
    result.CompilerMessages = m_LastCompilerMessages.empty() ? nullptr : m_LastCompilerMessages.data();
    result.CompilerMessageCount = m_LastCompilerMessages.size();
    m_LastResult = result;
    return m_LastResult;
}

CKAS_STATUS ScriptApiDiagnostics::StoreResult(
    CKAngelScriptResult *out,
    CKAS_STATUS status,
    int angelScriptCode,
    const std::string &errorMessage,
    const std::string &stackTrace,
    const std::vector<CapturedScriptMessage> *compilerMessages) {
    CKAngelScriptResult result = MakeResult(status, angelScriptCode, errorMessage, stackTrace, compilerMessages);
    if (out) {
        *out = result;
    }
    return status;
}

const CKAngelScriptResult *ScriptApiDiagnostics::GetLastResult() const {
    return &m_LastResult;
}

void ScriptApiDiagnostics::BeginScriptMessageCapture() {
    m_CapturedScriptMessages.clear();
    m_CapturedCompilerMessages.clear();
    m_CapturingScriptMessages = true;
}

std::string ScriptApiDiagnostics::EndScriptMessageCapture(std::vector<CapturedScriptMessage> *messages) {
    m_CapturingScriptMessages = false;
    if (messages) {
        *messages = m_CapturedCompilerMessages;
    }
    return m_CapturedScriptMessages;
}

bool ScriptApiDiagnostics::IsCapturingScriptMessages() const {
    return m_CapturingScriptMessages;
}

void ScriptApiDiagnostics::CaptureScriptMessage(const std::string &formatted, CapturedScriptMessage message) {
    if (!m_CapturedScriptMessages.empty()) {
        m_CapturedScriptMessages += "\n";
    }
    m_CapturedScriptMessages += formatted;
    m_CapturedCompilerMessages.push_back(std::move(message));
}
