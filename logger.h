#ifndef LOGGER_H
#define LOGGER_H

#include <fstream>
#include <string>
#include "fileutils.h"

// Logger class definition
class Logger
{
public:
    // Log level enumeration
    enum LogLevel
    {
        INFO,
        WARNING,
        ERROR
    };

    // Constructor that takes a file path for the log file
    Logger(const std::string &filename);

    // Destructor to close the log file
    ~Logger();

    // Function to log a message with an optional log level (default is INFO)
    void log(const std::string &message, LogLevel level = INFO);

private:
    std::ofstream logFile; // Output file stream for logging

    // Helper function to convert log level enum to string
    std::string logLevelToString(LogLevel level);
};

#endif // LOGGER_H