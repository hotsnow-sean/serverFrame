#include "./log.h"
#include "./config.h"
#include <cstdarg>
#include <functional>
#include <iomanip>
#include <iostream>
#include <set>

namespace sylar {

const char *LogLevel::ToString(LogLevel::Level level) {
    switch (level) {
#define XX(name) \
    case Level::name: \
        return #name; \
        break;

        XX(Debug);
        XX(Info);
        XX(Warn);
        XX(Error);
        XX(Fatal);
#undef XX
    default:
        return "UNKNOW";
        break;
    }
}

LogLevel::Level LogLevel::FromString(const std::string &str) {
    if (str.size() < 4) return LogLevel::Unknow;
    std::string nstr(str.size(), 0);
    std::transform(str.begin(), str.end(), nstr.begin(), ::tolower);
    nstr.front() = ::toupper(nstr.front());
#define XX(name) \
    if (nstr == #name) { \
        return LogLevel::name; \
    }

    XX(Debug);
    XX(Info);
    XX(Warn);
    XX(Error);
    XX(Fatal);
    return LogLevel::Unknow;

#undef XX
}

void to_json(nlohmann::json &j, const LogLevel::Level &v) {
    j = sylar::LogLevel::ToString(v);
}

void from_json(const nlohmann::json &j, LogLevel::Level &v) {
    v = sylar::LogLevel::FromString(j.get<std::string>());
}

LogEventWrap::LogEventWrap(LogEvent::ptr e)
    : m_event(e) {
}

LogEventWrap::~LogEventWrap() {
    m_event->getLogger()->log(m_event->getLevel(), m_event);
}

void LogEvent::format(const char *fmt, ...) {
    va_list al;
    va_start(al, fmt);
    va_list tmp;
    va_copy(tmp, al);
    int len = 1 + vsnprintf(nullptr, 0, fmt, tmp);
    va_end(tmp);
    if (len > 0) {
        char *buf = (char *)malloc(len);
        vsprintf(buf, fmt, al);
        m_ss << buf;
        free(buf);
    }
    va_end(al);
}

std::stringstream &LogEventWrap::getSS() {
    return m_event->getSS();
}

class MessageFormatItem : public LogFormatter::FormatItem {
public:
    MessageFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getContent();
    }
};

class LevelFormatItem : public LogFormatter::FormatItem {
public:
    LevelFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        for (const char *l = LogLevel::ToString(level); *l != '\0'; ++l)
            os << (char)toupper(*l);
    }
};

class ElapseFormatItem : public LogFormatter::FormatItem {
public:
    ElapseFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getElapse();
    }
};

class NameFormatItem : public LogFormatter::FormatItem {
public:
    NameFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getLogger()->getName();
    }
};

class ThreadIdFormatItem : public LogFormatter::FormatItem {
public:
    ThreadIdFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getThreadId();
    }
};

class FiberIdFormatItem : public LogFormatter::FormatItem {
public:
    FiberIdFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getFiberId();
    }
};

class DateTimeFormatItem : public LogFormatter::FormatItem {
public:
    DateTimeFormatItem(const std::string &format = "%Y-%m-%d %H:%M:%S")
        : m_format(format) {
        if (m_format.empty()) {
            m_format = "%Y-%m-%d %H:%M:%S";
        }
    }

    void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        time_t time = event->getTime();
        struct tm *timeinfo = localtime(&time);
        char buf[64];
        strftime(buf, sizeof(buf), m_format.c_str(), timeinfo);
        os << buf;
    }

private:
    std::string m_format;
};

class FilenameFormatItem : public LogFormatter::FormatItem {
public:
    FilenameFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getFile();
    }
};

class LineFormatItem : public LogFormatter::FormatItem {
public:
    LineFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getLine();
    }
};

class NewLineFormatItem : public LogFormatter::FormatItem {
public:
    NewLineFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << std::endl;
    }
};

class StringFormatItem : public LogFormatter::FormatItem {
public:
    StringFormatItem(const std::string &str)
        : m_string(str) {}
    void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << m_string;
    }

private:
    std::string m_string;
};

class TabFormatItem : public LogFormatter::FormatItem {
public:
    TabFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << "\t";
    }
};

LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char *file, int32_t line, uint32_t elapse,
                   uint32_t thread_id, uint32_t fiber_id, uint64_t time)
    : m_file(file), m_line(line), m_elapse(elapse),
      m_threadId(thread_id), m_fiberId(fiber_id),
      m_time(time), m_logger(logger), m_level(level) {
}

Logger::Logger(const std::string &name)
    : m_name(name), m_level(LogLevel::Debug) {
    m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
}

void Logger::setFormatter(const std::string &val) {
    m_formatter.reset(new LogFormatter(val));
}

void Logger::setFormatter(LogFormatter::ptr val) {
    m_formatter = val;
}

LogFormatter::ptr Logger::getFormatter() const {
    return m_formatter;
}

void Logger::addAppender(LogAppender::ptr appender) {
    if (!appender->getFormatter()) {
        appender->setFormatter(m_formatter);
    }
    m_appenders.push_back(appender);
}

void Logger::delAppender(LogAppender::ptr appender) {
    for (auto it = m_appenders.begin(); it != m_appenders.end(); ++it) {
        if (*it == appender) {
            m_appenders.erase(it);
            break;
        }
    }
}

void Logger::clearAppender() {
    m_appenders.clear();
}

void Logger::log(LogLevel::Level level, LogEvent::ptr event) {
    if (level >= m_level) {
        auto self = shared_from_this();
        if (!m_appenders.empty()) {
            for (auto &i : m_appenders) {
                i->log(self, level, event);
            }
        } else if (m_root) {
            m_root->log(level, event);
        }
    }
}

void Logger::debug(LogEvent::ptr event) {
    log(LogLevel::Debug, event);
}

void Logger::info(LogEvent::ptr event) {
    log(LogLevel::Info, event);
}

void Logger::warn(LogEvent::ptr event) {
    log(LogLevel::Warn, event);
}

void Logger::error(LogEvent::ptr event) {
    log(LogLevel::Error, event);
}

void Logger::fatal(LogEvent::ptr event) {
    log(LogLevel::Fatal, event);
}

FileLogAppender::FileLogAppender(const std::string &filename)
    : m_filename(filename) {
    reopen();
}

void FileLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
    if (level >= m_level) {
        m_filestream << m_formatter->format(logger, level, event);
    }
}

bool FileLogAppender::reopen() {
    if (m_filestream) {
        m_filestream.close();
    }
    m_filestream.open(m_filename, std::ios::app);
    return !!m_filestream;
}

void StdoutLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
    if (level >= m_level) {
        std::cout << m_formatter->format(logger, level, event);
    }
}

LogFormatter::LogFormatter(const std::string &pattern)
    : m_pattern(pattern) {
    init();
}

std::string LogFormatter::format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
    std::stringstream ss;
    for (auto &i : m_items) {
        i->format(ss, logger, level, event);
    }
    return ss.str();
}

void LogFormatter::init() {
    // str format type
    std::vector<std::tuple<std::string, std::string, int>> vec;
    std::string nstr;
    for (size_t i = 0; i < m_pattern.size(); ++i) {
        if (m_pattern[i] != '%') {
            nstr.append(1, m_pattern[i]);
            continue;
        }
        if (i + 1 < m_pattern.size()) {
            if (m_pattern[i + 1] == '%') {
                nstr.append(1, '%');
                continue;
            }
        }
        size_t n = i + 1;
        int fmt_status = 0;
        size_t fmt_begin = 0;

        std::string str;
        std::string fmt;
        while (n < m_pattern.size()) {
            if (!fmt_status && !isalpha(m_pattern[n]) && m_pattern[n] != '{' && m_pattern[n] != '}') {
                str = m_pattern.substr(i + 1, n - i - 1);
                break;
            }
            if (fmt_status == 0) {
                if (m_pattern[n] == '{') {
                    str = m_pattern.substr(i + 1, n - i - 1);
                    fmt_status = 1; // 解析格式
                    fmt_begin = n;
                    n++;
                    continue;
                }
            }
            if (fmt_status == 1) {
                if (m_pattern[n] == '}') {
                    fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
                    fmt_status = 0;
                    n++;
                    break;
                }
            }
            n++;
            if (n == m_pattern.size()) {
                if (str.empty()) {
                    str = m_pattern.substr(i + 1);
                }
            }
        }
        if (fmt_status == 0) {
            if (!nstr.empty()) {
                vec.push_back(std::make_tuple(nstr, "", 0));
                nstr.clear();
            }
            vec.push_back(std::make_tuple(str, fmt, 1));
            i = n - 1;
        } else if (fmt_status == 1) {
            std::cout << "pattern parse error: " << m_pattern << " - " << m_pattern.substr(i) << std::endl;
            vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
        }
    }

    if (!nstr.empty()) {
        vec.push_back(std::make_tuple(nstr, "", 0));
    }

    static std::map<std::string, std::function<FormatItem::ptr(const std::string &str)>> s_format_items = {
#define XX(str, C) \
    { \
#str, [](const std::string &fmt) { return FormatItem::ptr(new C(fmt)); } \
    }

        XX(m, MessageFormatItem),
        XX(p, LevelFormatItem),
        XX(r, ElapseFormatItem),
        XX(c, NameFormatItem),
        XX(t, ThreadIdFormatItem),
        XX(n, NewLineFormatItem),
        XX(d, DateTimeFormatItem),
        XX(f, FilenameFormatItem),
        XX(l, LineFormatItem),
        XX(T, TabFormatItem),
        XX(F, FiberIdFormatItem)
#undef XX
    };

    for (auto &i : vec) {
        if (std::get<2>(i) == 0) {
            m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
        } else {
            auto it = s_format_items.find(std::get<0>(i));
            if (it == s_format_items.end()) {
                m_items.push_back(FormatItem::ptr(new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
                m_error = true;
            } else {
                m_items.push_back(it->second(std::get<1>(i)));
            }
        }

        // std::cout << "(" << std::get<0>(i) << ") - (" << std::get<1>(i) << ") - (" << std::get<2>(i) << ")" << std::endl;
    }
}

LogManager::LogManager() {
    m_root.reset(new Logger);
    m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));
    m_loggers["root"] = m_root;
    init();
}

std::shared_ptr<LogManager> LogManager::GetInstance() {
    static std::shared_ptr<LogManager> self(new LogManager);
    return self;
}

Logger::ptr LogManager::getLogger(const std::string &name) {
    auto it = m_loggers.find(name);
    if (it != m_loggers.end()) return it->second;
    Logger::ptr logger(new Logger(name));
    logger->m_root = m_root;
    m_loggers[name] = logger;
    return logger;
}

struct LogAppenderDefine {
    int type = 2; // 1 File 2 Stdout
    LogLevel::Level level = LogLevel::Unknow;
    std::string formatter;
    std::string file;

    bool operator==(const LogAppenderDefine &oth) const {
        return type == oth.type && level == oth.level && formatter == oth.formatter && file == oth.file;
    }
};

struct LogDefine {
    std::string name;
    LogLevel::Level level = LogLevel::Unknow;
    std::string formatter;
    std::vector<LogAppenderDefine> appenders;

    bool operator==(const LogDefine &oth) const {
        return name == oth.name && level == oth.level && formatter == oth.formatter && appenders == oth.appenders;
    }
    bool operator<(const LogDefine &oth) const {
        return name < oth.name;
    }
};

void to_json(nlohmann::json &j, const LogAppenderDefine &v) {
    j["type"] = v.type == 1 ? "FileLogAppender" : "StdoutLogAppender";
    if (v.level != LogLevel::Unknow) j["level"] = v.level;
    if (!v.formatter.empty()) j["formatter"] = v.formatter;
    if (!v.file.empty()) j["file"] = v.file;
}
void to_json(nlohmann::json &j, const LogDefine &v) {
    j["name"] = v.name;
    if (v.level != LogLevel::Unknow) j["level"] = v.level;
    if (!v.formatter.empty()) j["formatter"] = v.formatter;
    if (!v.appenders.empty()) j["appenders"] = v.appenders;
}

#define XX(j, v, key, is_type, prefix) \
    if (j.contains(#key)) { \
        if (j[#key].is_type()) \
            j[#key].get_to(v.key); \
        else \
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "config exception: " #prefix " " #key "'s type should " #is_type; \
    }

void from_json(const nlohmann::json &j, LogAppenderDefine &v) {
    if (j.contains("type")) {
        if (j["type"].is_string()) {
            std::string str = j["type"].get<std::string>();
            if (str == "FileLogAppender")
                v.type = 1;
            else if (str == "StdoutLogAppender")
                v.type = 2;
        } else
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "config exception: Appender type should be string";
    }
    XX(j, v, level, is_string, Appender);
    XX(j, v, formatter, is_string, Appender);
    XX(j, v, file, is_string, Appender);
}

void from_json(const nlohmann::json &j, LogDefine &v) {
    XX(j, v, name, is_string, Logs);
    XX(j, v, level, is_string, Logs);
    XX(j, v, formatter, is_string, Logs);
    XX(j, v, appenders, is_array, Logs);
}

#undef XX

sylar::ConfigVar<std::set<LogDefine>>::ptr g_log_defines =
    sylar::Config::Lookup("logs", std::set<LogDefine>(), "logs config");

struct LogIniter {
    LogIniter() {
        g_log_defines->addListerner(0xF1E231, [](const std::set<LogDefine> &old_value, const std::set<LogDefine> &new_value) {
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "on_logger_conf_changed";
            for (auto &i : new_value) {
                // 新增或者修改 logger
                auto logger = SYLAR_LOG_NAME(i.name);
                logger->setLevel(i.level);
                if (!i.formatter.empty()) logger->setFormatter(i.formatter);
                logger->clearAppender();
                for (auto &a : i.appenders) {
                    sylar::LogAppender::ptr ap;
                    if (a.type == 1)
                        ap.reset(new sylar::FileLogAppender(a.file));
                    else if (a.type == 2)
                        ap.reset(new sylar::StdoutLogAppender);
                    ap->setLevel(a.level);
                    if (!a.formatter.empty()) {
                        LogFormatter::ptr fmt(new LogFormatter(a.formatter));
                        if (!fmt->is_Error()) {
                            ap->setHasFormatter(fmt);
                        } else {
                            std::cout << "logger name=" << i.name << " formatter=" << a.formatter << " is invalid" << std::endl;
                        }
                    }
                    logger->addAppender(ap);
                }
            }
            for (auto &i : old_value) {
                auto it = new_value.find(i);
                if (it == new_value.end()) {
                    // 删除 logger
                    auto logger = SYLAR_LOG_NAME(i.name);
                    logger->setLevel((LogLevel::Level)100);
                    logger->clearAppender();
                }
            }
        });
    }
};

static LogIniter __log_init;

void LogManager::init() {
}

std::string LogManager::toJsonString() const {
    nlohmann::json j;
    for (auto &l : m_loggers) {
        LogDefine ld;
        ld.name = l.first;
        ld.level = l.second->m_level;
        ld.formatter = l.second->m_formatter->getPattern();
        for (auto &a : l.second->m_appenders) {
            LogAppenderDefine lad;
            if (typeid(*a) == typeid(FileLogAppender)) {
                lad.type = 1;
                lad.file = std::dynamic_pointer_cast<FileLogAppender>(a)->getFileName();
            } else {
                lad.type = 2;
            }
            lad.level = a->getLevel();
            if (a->hasFormatter()) lad.formatter = a->getFormatter()->getPattern();
            ld.appenders.push_back(lad);
        }
        j.push_back(ld);
    }
    std::stringstream ss;
    ss << std::setw(4) << j;
    return ss.str();
}

} // namespace sylar