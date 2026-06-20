#include "ScriptSelfTests.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include "ScriptInfo.h"

#if CKAS_ENABLE_API_EXPORT

namespace {

class ScopedEnvVar {
public:
    ScopedEnvVar(const char *name, const char *value)
        : m_Name(name) {
        const char *oldValue = std::getenv(name);
        if (oldValue) {
            m_HadOldValue = true;
            m_OldValue = oldValue;
        }
        _putenv_s(name, value ? value : "");
    }

    ~ScopedEnvVar() {
        if (m_HadOldValue) {
            _putenv_s(m_Name.c_str(), m_OldValue.c_str());
        } else {
            _putenv_s(m_Name.c_str(), "");
        }
    }

    ScopedEnvVar(const ScopedEnvVar &) = delete;
    ScopedEnvVar &operator=(const ScopedEnvVar &) = delete;

private:
    std::string m_Name;
    bool m_HadOldValue = false;
    std::string m_OldValue;
};

std::string ReadFile(const std::filesystem::path &path) {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream out;
    out << in.rdbuf();
    return out.str();
}

} // namespace

bool RunScriptInfoExportSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "ScriptInfo export self-test requires an AngelScript engine.";
        return false;
    }

    const std::filesystem::path outputPath =
        std::filesystem::temp_directory_path() / "__ckas_script_info_export_self_test.json";
    std::error_code ec;
    std::filesystem::remove(outputPath, ec);
    std::filesystem::remove(outputPath.string() + ".stage", ec);

    {
        const ScopedEnvVar exportPath("CKAS_EXPORT_SCRIPT_API", outputPath.string().c_str());
        if (!ExportScriptApiIfRequested(engine, error)) {
            return false;
        }
    }

    if (!std::filesystem::exists(outputPath)) {
        error = "ScriptInfo export self-test did not create the export file.";
        return false;
    }

    const std::string document = ReadFile(outputPath);
    std::filesystem::remove(outputPath, ec);
    std::filesystem::remove(outputPath.string() + ".stage", ec);

    if (document.find("\"schemaVersion\": 1") == std::string::npos ||
        document.find("\"angelScriptVersion\"") == std::string::npos ||
        document.find("\"globalFunctions\"") == std::string::npos ||
        document.find("\"objectTypes\"") == std::string::npos) {
        error = "ScriptInfo export self-test found an incomplete export document.";
        return false;
    }
    if (document.find("\"engineProperties\"") == std::string::npos ||
        document.find("\"funcdefs\"") == std::string::npos) {
        error = "ScriptInfo export self-test found missing export sections.";
        return false;
    }

    return true;
}

#endif
