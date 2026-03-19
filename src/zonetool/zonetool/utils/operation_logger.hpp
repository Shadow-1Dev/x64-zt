#pragma once

#include "taskbar.hpp"

#include <utils/string.hpp>

#include <windows.h>

#include <string>
#include <mutex>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>

namespace zonetool::operation_logger
{
	enum class message_level
	{
		info,
		warning,
		error,
		fatal,
	};

	namespace detail
	{
		inline std::mutex logger_mutex{};
		inline std::ofstream logger_file{};
		inline bool operation_active = false;

		inline std::string safe_filename(const std::string& name)
		{
			std::string result = name;
			if (result.empty())
			{
				result = "unknown_zone";
			}

			for (char& c : result)
			{
				switch (c)
				{
				case '<':
				case '>':
				case ':':
				case '"':
				case '/':
				case '\\':
				case '|':
				case '?':
				case '*':
					c = '_';
					break;
				default:
					break;
				}
			}

			return result;
		}

		inline std::filesystem::path get_executable_directory()
		{
			char path[MAX_PATH] = {};
			constexpr auto path_capacity = static_cast<DWORD>(sizeof(path) / sizeof(path[0]));
			const auto length = GetModuleFileNameA(nullptr, path, path_capacity);
			if (length == 0 || length == path_capacity)
			{
				return std::filesystem::current_path();
			}

			return std::filesystem::path(std::string(path, length)).parent_path();
		}

		inline std::string get_timestamp(const char* format)
		{
			auto now = std::chrono::system_clock::now();
			auto t = std::chrono::system_clock::to_time_t(now);

			std::tm local_time{};
			localtime_s(&local_time, &t);

			std::ostringstream stream{};
			stream << std::put_time(&local_time, format);
			return stream.str();
		}

		inline std::string get_level_name(const message_level level)
		{
			switch (level)
			{
			case message_level::info:
				return "INFO";
			case message_level::warning:
				return "WARNING";
			case message_level::error:
				return "ERROR";
			case message_level::fatal:
				return "FATAL";
			default:
				return "INFO";
			}
		}

		inline std::string format_message(const char* format, va_list ap)
		{
			va_list ap_copy{};
			va_copy(ap_copy, ap);
			const auto length = std::vsnprintf(nullptr, 0, format, ap_copy);
			va_end(ap_copy);

			if (length <= 0)
			{
				return {};
			}

			std::string result(static_cast<std::size_t>(length), '\0');
			std::vsnprintf(result.data(), result.size() + 1, format, ap);
			return result;
		}

		inline void append_to_operation_log_unlocked(const std::string& line)
		{
			if (!operation_active || !logger_file.is_open())
			{
				return;
			}

			logger_file << '[' << get_timestamp("%H:%M:%S") << "] " << line << '\n';
			logger_file.flush();
		}

		inline void close_operation_unlocked(const bool success)
		{
			if (!operation_active)
			{
				return;
			}

			append_to_operation_log_unlocked(success
				? "[ INFO ] Operation finished successfully."
				: "[ ERROR ] Operation ended with errors.");

			if (logger_file.is_open())
			{
				logger_file.close();
			}

			operation_active = false;
		}
	}

	inline void begin_operation(const std::string& operation, const std::string& zone_name)
	{
		std::lock_guard<std::mutex> lock(detail::logger_mutex);

		detail::close_operation_unlocked(false);

		const auto log_directory = detail::get_executable_directory() / "zonetool_log";
		std::error_code ec{};
		std::filesystem::create_directories(log_directory, ec);
		if (ec)
		{
			return;
		}

		const auto file_name = detail::safe_filename(zone_name) + "_" + detail::get_timestamp("%d-%m-%Y__%H-%M-%S") + ".log";
		const auto file_path = log_directory / file_name;

		detail::logger_file.open(file_path, std::ios::out | std::ios::trunc);
		if (!detail::logger_file.is_open())
		{
			return;
		}

		detail::operation_active = true;
		detail::append_to_operation_log_unlocked("[ INFO ] Operation: " + operation);
		detail::append_to_operation_log_unlocked("[ INFO ] Zone/Fastfile: " + zone_name);
		detail::append_to_operation_log_unlocked("[ INFO ] Log file: " + file_path.string());
	}

	inline void end_operation(const bool success)
	{
		std::lock_guard<std::mutex> lock(detail::logger_mutex);
		if (!detail::operation_active)
		{
			return;
		}

		detail::close_operation_unlocked(success);
		MessageBeep(success ? MB_OK : MB_ICONHAND);
	}

	inline void record_asset(const std::string& type, const std::string& name)
	{
		std::lock_guard<std::mutex> lock(detail::logger_mutex);
		detail::append_to_operation_log_unlocked("[ ASSET ] " + type + "," + name);
	}

	inline void write_log_message(message_level level, const char* function_name, const char* format, ...)
	{
		va_list ap{};
		va_start(ap, format);
		const auto message = detail::format_message(format, ap);
		va_end(ap);

		const auto level_name = detail::get_level_name(level);
		const auto line = utils::string::va("[ %s ][ %s ]: %s", level_name.data(), function_name, message.data());

		switch (level)
		{
		case message_level::warning:
			printf("\x1b[33m%s\x1b[0m\n", line);
			break;
		case message_level::error:
			printf("\x1b[31m%s\x1b[0m\n", line);
			break;
		case message_level::fatal:
			printf("\x1b[1m\x1b[7m\x1b[31m%s\x1b[0m\n", line);
			break;
		case message_level::info:
		default:
			printf("%s\n", line);
			break;
		}

		std::lock_guard<std::mutex> lock(detail::logger_mutex);
		detail::append_to_operation_log_unlocked(line);
	}

	[[noreturn]] inline void fatal(const char* function_name, const char* format, ...)
	{
		va_list ap{};
		va_start(ap, format);
		const auto message = detail::format_message(format, ap);
		va_end(ap);

		const auto line = utils::string::va("[ FATAL ][ %s ]: %s", function_name, message.data());
		printf("\x1b[1m\x1b[7m\x1b[31m%s\x1b[0m\n", line);

		{
			std::lock_guard<std::mutex> lock(detail::logger_mutex);
			detail::append_to_operation_log_unlocked(line);
		}

		zonetool::taskbar::set_error();

		const auto last_error = GetLastError();
		const auto popup_message = utils::string::va(
			"Oops! An unexpected error occured.\n\n"
			"Error:\n%s"
			"\n\nZoneTool must be restarted.\n"
			"Last Win32 error: 0x%08X (%u)",
			message.data(), last_error, last_error);

		MessageBoxA(nullptr, popup_message, "ZoneTool ERROR", MB_ICONERROR);
		std::quick_exit(EXIT_FAILURE);
	}

	class operation_scope final
	{
	public:
		operation_scope(const std::string& operation, const std::string& zone_name)
			: success_(false)
		{
			begin_operation(operation, zone_name);
		}

		~operation_scope()
		{
			end_operation(this->success_);
		}

		operation_scope(const operation_scope&) = delete;
		operation_scope& operator=(const operation_scope&) = delete;

		void mark_success()
		{
			this->success_ = true;
		}

	private:
		bool success_;
	};
}
