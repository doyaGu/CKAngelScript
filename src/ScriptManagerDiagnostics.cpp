#include "ScriptManager.h"

#include <string>
#include <vector>

CKAngelScriptResult ScriptManager::MakeResult(CKAS_STATUS status,
                                              int angelScriptCode,
                                              const std::string &errorMessage,
                                              const std::string &stackTrace,
                                              const std::vector<CapturedScriptMessage> *compilerMessages) {
    return m_Diagnostics.MakeResult(status, angelScriptCode, errorMessage, stackTrace, compilerMessages);
}

CKAS_STATUS ScriptManager::StoreResult(CKAngelScriptResult *out,
                                       CKAS_STATUS status,
                                       int angelScriptCode,
                                       const std::string &errorMessage,
                                       const std::string &stackTrace,
                                       const std::vector<CapturedScriptMessage> *compilerMessages) {
    return m_Diagnostics.StoreResult(out, status, angelScriptCode, errorMessage, stackTrace, compilerMessages);
}

const CKAngelScriptResult *ScriptManager::GetLastResult() const {
    return m_Diagnostics.GetLastResult();
}

CKAS_STATUS ScriptManager::StoreApiResult(CKAngelScriptResult *out,
                                          CKAS_STATUS status,
                                          int angelScriptCode,
                                          const char *errorMessage,
                                          const char *stackTrace) {
    return StoreResult(out,
                       status,
                       angelScriptCode,
                       std::string(errorMessage ? errorMessage : ""),
                       std::string(stackTrace ? stackTrace : ""));
}

void ScriptManager::BeginScriptMessageCapture() {
    m_Diagnostics.BeginScriptMessageCapture();
}

std::string ScriptManager::EndScriptMessageCapture(std::vector<CapturedScriptMessage> *messages) {
    return m_Diagnostics.EndScriptMessageCapture(messages);
}
