#pragma once

#include "memory.hpp"

#include "io/filesystem.hpp"
#include "io/assetmanager.hpp"

#include "csv.hpp"
#include "taskbar.hpp"
#include "operation_logger.hpp"

#include "game/mode.hpp"
#include "game/shared.hpp"

#include <utils/memory.hpp>
#include <windows.h>
#include <cstdio>
#include <vector>
#include <cstdint>

#define ANSI_RESET   "\x1b[0m"

#define ANSI_RED     "\x1b[31m"
#define ANSI_YELLOW  "\x1b[33m"

#define ANSI_BOLD    "\x1b[1m"
#define ANSI_INVERT  "\x1b[7m"

namespace nlohmann
{
	static inline std::vector<std::uint8_t> get_object_bytes(json object)
	{
		if (!object.is_object())
		{
			throw (detail::concat("type must be object, but is ", object.type_name()));
		}

		auto bytes = object["bytes"];
		if (bytes.is_null())
		{
			throw ("object doesn't have \"bytes\" field");
		}

		std::vector<std::uint8_t> m_bytes(bytes.size());
		for (size_t i = 0; i < bytes.size(); i++)
		{
			m_bytes[i] = bytes[i].get<std::uint8_t>();
		}

		return m_bytes;
	}
}

#define MAX_ZONE_SIZE (1024ull * 1024ull * 1024ull) * 2ull
#define MAX_MEM_SIZE  (1024ull * 1024ull * 1024ull) * 2ull

#define ZONETOOL_INFO(...) \
	zonetool::operation_logger::write_log_message( \
		zonetool::operation_logger::message_level::info, \
		zonetool::strip_template(__FUNCTION__), \
		__VA_ARGS__)

#define ZONETOOL_WARNING(...) \
	zonetool::operation_logger::write_log_message( \
		zonetool::operation_logger::message_level::warning, \
		zonetool::strip_template(__FUNCTION__), \
		__VA_ARGS__)

#define ZONETOOL_ERROR(...) \
	zonetool::operation_logger::write_log_message( \
		zonetool::operation_logger::message_level::error, \
		zonetool::strip_template(__FUNCTION__), \
		__VA_ARGS__)

#define ZONETOOL_FATAL(...) \
	zonetool::operation_logger::fatal( \
		zonetool::strip_template(__FUNCTION__), \
		__VA_ARGS__)

namespace zonetool
{
	struct zonetool_globals_t
	{
		bool verify;
		bool dump;
		bool dump_referenced;
		bool dump_csv;
		game::game_mode target_game;
		filesystem::file csv_file;
	};
}
