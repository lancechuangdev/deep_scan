#include "logger.h"

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
        // Get current time with milliseconds
        auto now = std::chrono::system_clock::now();
        auto nowTimeT = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        // Format the time
        std::tm *nowTm = std::localtime(&nowTimeT);
        char timeStr[80];
        std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", nowTm);

        // Write the log message to the file with milliseconds
        logFile << "[" << timeStr << "." << std::setw(3) << std::setfill('0') << ms.count() << "] "
                << "[" << logLevelToString(level) << "] " << message << std::endl;
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