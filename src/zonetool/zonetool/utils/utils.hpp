#pragma once

#include "memory.hpp"

#include "io/filesystem.hpp"
#include "io/assetmanager.hpp"

#include "csv.hpp"
#include "taskbar.hpp"

#include "game/mode.hpp"
#include "game/shared.hpp"

#include <utils/memory.hpp>
#include <windows.h>
#include <cstdio>
#include <vector>
#include <cstdint>

// ======================================================
// ANSI COLORS (Windows 10+)
// ======================================================
#define ANSI_RESET   "\x1b[0m"

#define ANSI_RED     "\x1b[31m"
#define ANSI_YELLOW  "\x1b[33m"

#define ANSI_BOLD    "\x1b[1m"
#define ANSI_INVERT  "\x1b[7m"

// ======================================================
// JSON HELPERS
// ======================================================
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

// ======================================================
// LIMITS
// ======================================================
#define MAX_ZONE_SIZE (1024ull * 1024ull * 1024ull) * 2ull
#define MAX_MEM_SIZE  (1024ull * 1024ull * 1024ull) * 2ull

// ======================================================
// LOG MACROS
// ======================================================

// INFO ó »œÊ‰ √·Ê«‰ (⁄«œÌ)
#define ZONETOOL_INFO(__FMT__, ...) \
	printf("[ INFO ][ %s ]: " __FMT__ "\n", \
	zonetool::strip_template(__FUNCTION__), __VA_ARGS__)

// WARNING ó √’›—
#define ZONETOOL_WARNING(__FMT__, ...) \
	printf(ANSI_YELLOW "[ WARNING ][ %s ]: " __FMT__ ANSI_RESET "\n", \
	zonetool::strip_template(__FUNCTION__), __VA_ARGS__)

// ERROR ó √Õ„—
#define ZONETOOL_ERROR(__FMT__, ...) \
	printf(ANSI_RED "[ ERROR ][ %s ]: " __FMT__ ANSI_RESET "\n", \
	zonetool::strip_template(__FUNCTION__), __VA_ARGS__)

// FATAL ó √Õ„— €«„ﬁ + ⁄ﬂ”Ì
#define ZONETOOL_FATAL(__FMT__, ...) \
	printf(ANSI_BOLD ANSI_INVERT ANSI_RED \
	"[ FATAL ][ %s ]: " __FMT__ ANSI_RESET "\n", \
	zonetool::strip_template(__FUNCTION__), __VA_ARGS__); \
	zonetool::taskbar::set_error(); \
	MessageBoxA(nullptr, \
	&utils::string::va( \
	"Oops! An unexpected error occured.\n\n" \
	"Error:\n" __FMT__ \
	"\n\nZoneTool must be restarted.\n" \
	"Last Win32 error: 0x%08X (%u)", \
	__VA_ARGS__, GetLastError(), GetLastError())[0], \
	nullptr, MB_ICONERROR); \
	std::quick_exit(EXIT_FAILURE)

// ======================================================
// GLOBALS
// ======================================================
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
