#pragma once
#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <vector>

enum LogType
{
	LOG_INFO = 0,
	LOG_WARNING,
	LOG_ERROR,
};

struct LogEntry
{
	LogType type;
	std::string message;
};

class Logger
{
public:

	// Logger() noexcept;
	static std::string CurrentDateTimeToString() noexcept;
	
	static void Log(const std::string& message) noexcept;
	static void Warning(const std::string& message) noexcept;
	static void Error(const std::string& message) noexcept;

	// one big container that contains all the messages
	static std::vector<LogEntry> messagesStack;
};

#endif // LOGGER_H
