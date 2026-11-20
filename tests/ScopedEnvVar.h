#pragma once

#include <cstdlib>
#include <optional>
#include <string>

class ScopedEnvVar {
public:
    ScopedEnvVar(const char *name, const std::string &value) : m_Name(name) {
        if (const auto *previous = std::getenv(name)) {
            m_Previous = previous;
        }
        Set(value.c_str());
    }

    ~ScopedEnvVar() { Set(m_Previous ? m_Previous->c_str() : nullptr); }

    ScopedEnvVar(const ScopedEnvVar &) = delete;
    ScopedEnvVar &operator=(const ScopedEnvVar &) = delete;

private:
    void Set(const char *value) const {
#ifdef _WIN32
        _putenv_s(m_Name.c_str(), value ? value : "");
#else
        if (value) {
            setenv(m_Name.c_str(), value, 1);
        } else {
            unsetenv(m_Name.c_str());
        }
#endif
    }

    std::string m_Name;
    std::optional<std::string> m_Previous;
};
