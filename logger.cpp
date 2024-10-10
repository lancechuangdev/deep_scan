#include "logger.h"
#include <iostream>
#include <ctime>

// Constructor that opens the log file
Logger::Logger(const std::string &filename)
{
    if (!FileUtils::createFile(filename))
    {
        std::cerr << "Error: Unable to create log file: " << filename << std::endl;
    }

    logFile.open(filename, std::ios::out | std::ios::app); // Append mode
    if (!logFile)
    {
        std::cerr << "Error: Unable to open log file: " << filename << std::endl;
    }
}

// Destructor that closes the log file
Logger::~Logger()
{
    if (logFile.is_open())
    {
        logFile.close();
    }
}

// Function to log a message with a specific log level
void Logger::log(const std::string &message, LogLevel level)
{
    if (logFile.is_open())
    {
        // Get current time
        std::time_t now = std::time(nullptr);
        char timeStr[100];
        std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", std::localtime(&now));

        // Write the log message to the file
        logFile << "[" << timeStr << "] [" << logLevelToString(level) << "] " << message << std::endl;
    }
    else
    {
        std::cerr << "Error: Log file is not open." << std::endl;
    }
}

// Helper function to convert the log level enum to a string
std::string Logger::logLevelToString(LogLevel level)
{
    switch (level)
    {
        case INFO:
            return "INFO";
        case WARNING:
            return "WARNING";
        case ERROR:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}