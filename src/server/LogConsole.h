// (C) 2015 Tim Gurto

#ifndef LOG_CONSOLE_H
#define LOG_CONSOLE_H

#include <fstream>
#include <iostream>
#include <string>

#include "../Log.h"

// A message log that writes to the console.
class LogConsole : public Log{
public:
    LogConsole(const std::string &logFileName = "");
    ~LogConsole() override;

    void operator()(const std::string &message, const Color &color = Color::YELLOW) override;

    void quiet(bool b = true){ _quiet = b; }

    template<typename T>
    LogConsole &operator<<(const T &val) {
        if (_quiet)
            return *this;

        std::cout << val;
        if (_logFile.is_open())
            _logFile << val;
        return *this;
    }
    LogConsole &operator<<(const std::string &val) override;
    LogConsole &operator<<(const LogSpecial &val) override;
    LogConsole &operator<<(const Color &c) override;

private:
    std::ofstream _logFile;
    bool _quiet; // If true, suppress all messages.
};

#endif
