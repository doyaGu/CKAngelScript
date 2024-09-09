#include "Logger.h"

#include <ctime>

struct LogInfo {
    va_list ap;
    const char *format;
    struct tm *time;
    void *userdata;
    LogLevel level;

    static void Init(LogInfo &info, void *userdata) {
        if (!info.time) {
            time_t t = std::time(nullptr);
            info.time = localtime(&t);
        }
        info.userdata = userdata;
    }
};

static const char *g_LevelStrings[6] = {
    "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

static void FileCallback(const LogInfo &info) {
    char buf[64];
    buf[strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", info.time)] = '\0';
    FILE *fp = (FILE *) info.userdata;
    fprintf(fp, "%s [%s]: ", buf, g_LevelStrings[info.level]);
    vfprintf(fp, info.format, info.ap);
    fprintf(fp, "\n");
    fflush(fp);
}

bool Logger::Init(const char *filename, LogLevel level) {
    if (!filename || filename[0] == '\0')
        return false;

    FILE *fp = fopen(filename, "a");
    if (!fp)
        return false;

    m_File = fp;
    m_Level = level;
    return true;
}

Logger &Logger::Get() {
    static Logger instance;
    return instance;
}

Logger::~Logger() {
    fclose(m_File);
}

void Logger::Log(LogLevel level, const char *format, va_list args) {
    if (level >= m_Level && level < LOG_LEVEL_OFF) {
        LogInfo info = {args, format, nullptr, nullptr, level};
        LogInfo::Init(info, m_File);
        FileCallback(info);
    }
}

Logger::Logger() : m_Level(LOG_LEVEL_TRACE), m_File(nullptr) {}