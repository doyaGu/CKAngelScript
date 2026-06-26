#ifndef CK_SCRIPT_API_DIAGNOSTICS_H
#define CK_SCRIPT_API_DIAGNOSTICS_H

#include <string>
#include <vector>

#include "CKAngelScript.h"

struct CapturedScriptMessage {
    std::string Section;
    int Row = 0;
    int Column = 0;
    CKAS_MESSAGETYPE Type = CKAS_MESSAGE_INFORMATION;
    std::string Message;
};

class ScriptApiDiagnostics {
public:
    CKAngelScriptResult MakeResult(CKAS_STATUS status,
                                   int angelScriptCode = 0,
                                   const std::string &errorMessage = std::string(),
                                   const std::string &stackTrace = std::string(),
                                   const std::vector<CapturedScriptMessage> *compilerMessages = nullptr);
    CKAS_STATUS StoreResult(CKAngelScriptResult *out,
                            CKAS_STATUS status,
                            int angelScriptCode = 0,
                            const std::string &errorMessage = std::string(),
                            const std::string &stackTrace = std::string(),
                            const std::vector<CapturedScriptMessage> *compilerMessages = nullptr);

    const CKAngelScriptResult *GetLastResult() const;

    void BeginScriptMessageCapture();
    std::string EndScriptMessageCapture(std::vector<CapturedScriptMessage> *messages = nullptr);
    bool IsCapturingScriptMessages() const;
    void CaptureScriptMessage(const std::string &formatted, CapturedScriptMessage message);

private:
    CKAngelScriptResult m_LastResult = {sizeof(CKAngelScriptResult), CKAS_OK, 0, nullptr, nullptr, nullptr, 0};
    std::string m_LastErrorMessage;
    std::string m_LastStackTrace;
    std::vector<CapturedScriptMessage> m_LastCompilerMessageStorage;
    std::vector<CKAngelScriptCompilerMessage> m_LastCompilerMessages;
    bool m_CapturingScriptMessages = false;
    std::string m_CapturedScriptMessages;
    std::vector<CapturedScriptMessage> m_CapturedCompilerMessages;
};

#endif // CK_SCRIPT_API_DIAGNOSTICS_H
