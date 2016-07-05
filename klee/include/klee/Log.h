#ifndef CHECKER_LOG_H_
#define CHECKER_LOG_H_
#include <string>
#include <cstdio>
#include <cstdarg>

class Log {
    std::string log_name;

public:
#define DEFINE_LEVEL(name, level) \
    void name(const char *format, ...) { \
        fprintf(stderr, "<%s> %5s: ", log_name.c_str(), #level); \
        va_list args; \
        va_start(args, format); \
        vfprintf(stderr, format, args); \
        fprintf(stderr, "\n"); \
        va_end(args); \
    }

    enum LogLevels {
        LOG_VERBOSE = 0,
        LOG_DEBUG = 5,
        LOG_INFO = 10,
        LOG_WARN = 15,
        LOG_ERR = 20
    };

    DEFINE_LEVEL(verbose, VERBOSE);
    DEFINE_LEVEL(debug, DEBUG);
    DEFINE_LEVEL(info, INFO);
    DEFINE_LEVEL(warn, WARN);
    DEFINE_LEVEL(err, ERR);

    Log(const std::string &_log_name = "Log") : log_name(_log_name) {}

};

#endif // CHECKER_LOG_H_
