#ifndef CK_LOGGER_H
#define CK_LOGGER_H

#include <cstdarg>
#include <cstdio>

#define LOG_TRACE(...) ::Logger::Get().Trace(__VA_ARGS__)
#define LOG_DEBUG(...) ::Logger::Get().Debug(__VA_ARGS__)
#define LOG_INFO(...)  ::Logger::Get().Info(__VA_ARGS__)
#define LOG_WARN(...)  ::Logger::Get().Warn(__VA_ARGS__)
#define LOG_ERROR(...) ::Logger::Get().Error(__VA_ARGS__)
#define LOG_FATAL(...) ::Logger::Get().Fatal(__VA_ARGS__)

typedef enum LogLevel {
    LOG_LEVEL_TRACE = 0,
    LOG_LEVEL_DEBUG = 1,
    LOG_LEVEL_INFO = 2,
    LOG_LEVEL_WARN = 3,
    LOG_LEVEL_ERROR = 4,
    LOG_LEVEL_FATAL = 5,
    LOG_LEVEL_OFF = 6
} LogLevel;

class Logger {
public:
    static Logger &Get();

    Logger(const Logger &rhs) = delete;
    Logger(Logger &&rhs) noexcept = delete;

    ~Logger();

    Logger &operator=(const Logger &rhs) = delete;
    Logger &operator=(Logger &&rhs) noexcept = delete;

    bool Init(const char *filename, LogLevel level = LOG_LEVEL_INFO);

    void SetLevel(LogLevel level) {
        if (level >= LOG_LEVEL_TRACE && level <= LOG_LEVEL_OFF)
            m_Level = level;
    }

    LogLevel GetLevel() const { return m_Level; }

    void Log(LogLevel level, const char *format, va_list args);

    void Trace(const char *format, ...) {
        va_list args;
        va_start(args, format);
        Log(LOG_LEVEL_TRACE, format, args);
        va_end(args);
    }

    void Debug(const char *format, ...) {
        va_list args;
        va_start(args, format);
        Log(LOG_LEVEL_DEBUG, format, args);
        va_end(args);
    }

    void Info(const char *format, ...) {
        va_list args;
        va_start(args, format);
        Log(LOG_LEVEL_INFO, format, args);
        va_end(args);
    }

    void Warn(const char *format, ...) {
        va_list args;
        va_start(args, format);
        Log(LOG_LEVEL_WARN, format, args);
        va_end(args);
    }

    void Error(const char *format, ...) {
        va_list args;
        va_start(args, format);
        Log(LOG_LEVEL_ERROR, format, args);
        va_end(args);
    }

    void Fatal(const char *format, ...) {
        va_list args;
        va_start(args, format);
        Log(LOG_LEVEL_FATAL, format, args);
        va_end(args);
    }

private:
    Logger();

    LogLevel m_Level;
    FILE *m_File;
};

#endif // CK_LOGGER_H