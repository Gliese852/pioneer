// Copyright © 2008-2025 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "Game.h"
#include "Pi.h"
#include "buildopts.h"
#include "core/OS.h"
#include "galaxy/Galaxy.h"
#include "galaxy/GalaxyGenerator.h"
#include "EnumStrings.h"
#include "utils.h"
#include "versioningInfo.h"
#include "lua/Lua.h"
#include "lua/LuaNameGen.h"
#include "lua/LuaObject.h"

#include <SDL.h>
#include <cstdio>
#include <cstdlib>

enum RunMode {
	MODE_GAME,
	MODE_GALAXYDUMP,
	MODE_START_AT,
	MODE_VERSION,
	MODE_USAGE,
	MODE_USAGE_ERROR
};

static RunMode parse_run_mode(int argc, char **argv)
{
	if (argc < 2) return MODE_GAME;

	const char switchchar = argv[1][0];

	if (!(switchchar == '-' || switchchar == '/')) {
		return MODE_USAGE_ERROR;
	}

	auto modeopt = std::string(argv[1]).substr(1);

	if (modeopt == "game" || modeopt == "g") {
		return MODE_GAME;
	}

	if (modeopt == "galaxydump" || modeopt == "gd") {
		return MODE_GALAXYDUMP;
	}

	if (modeopt.find("startat", 0, 7) != std::string::npos ||
		modeopt.find("sa", 0, 2) != std::string::npos) {
		return MODE_START_AT;
	}

	if (modeopt == "version" || modeopt == "v") {
		return MODE_VERSION;
	}

	if (modeopt == "help" || modeopt == "h" || modeopt == "?") {
		return MODE_USAGE;
	}

	return MODE_USAGE_ERROR;
}

class GalaxyDumpApp : public Application
{
	void OnStartup() override
	{
		Lua::Init(GetTaskGraph()->GetJobQueue());
		Pi::luaNameGen = new LuaNameGen(Lua::manager);

		EnumStrings::Init();
		GalacticEconomy::Init();
		LuaObject<SystemBody>::RegisterClass();
	}

	void OnShutdown() override
	{
		delete Pi::luaNameGen;
		Lua::Uninit();
	}
};

extern "C" int main(int argc, char **argv)
{
#ifdef PIONEER_PROFILER
	Profiler::detect(argc, argv);
#endif

	OS::SetDPIAware();

	RunMode mode = parse_run_mode(argc, argv);

	int pos = 2;
	long int radius = 4;
	long int sx = 0, sy = 0, sz = 0;
	std::string filename;
	SystemPath startPath(0, 0, 0, 0, 0);

	switch (mode) {

	case MODE_GALAXYDUMP: {
		if (argc < 3) {
			Output("pioneer: galaxy dump requires a filename\n");
			break;
		}
		filename = argv[pos];
		++pos;
		if (argc > pos) { // radius (optional)
			char *end = nullptr;
			radius = std::strtol(argv[pos], &end, 0);
			if (end == nullptr || *end != 0 || radius < 0 || radius > 10000) {
				Output("pioneer: invalid radius: %s\n", argv[pos]);
				break;
			}
			++pos;
		}
		if (argc > pos) { // center of dump (three comma separated coordinates, optional)
			char *end = nullptr;
			sx = std::strtol(argv[pos], &end, 0);
			if (end == nullptr || *end != ',' || sx < -10000 || sx > 10000) {
				Output("pioneer: invalid center: %s\n", argv[pos]);
				break;
			}
			sy = std::strtol(end + 1, &end, 0);
			if (end == nullptr || *end != ',' || sy < -10000 || sy > 10000) {
				Output("pioneer: invalid center: %s\n", argv[pos]);
				break;
			}
			sz = std::strtol(end + 1, &end, 0);
			if (end == nullptr || *end != 0 || sz < -10000 || sz > 10000) {
				Output("pioneer: invalid center: %s\n", argv[pos]);
				break;
			}
			++pos;
		}

		GalaxyDumpApp app;
		app.Startup(); // TaskGraph, FileSystem
		FILE *file = filename == "-" ? stdout : fopen(filename.c_str(), "w");
		if (file == nullptr) {
			Output("pioneer: could not open \"%s\" for writing: %s\n", filename.c_str(), strerror(errno));
			break;
		}
		RefCountedPtr<Galaxy> galaxy = GalaxyGenerator::Create();
		galaxy->Dump(file, sx, sy, sz, radius);
		if (filename != "-" && fclose(file) != 0) {
			Output("pioneer: writing to \"%s\" failed: %s\n", filename.c_str(), strerror(errno));
		}
		app.Shutdown();
		break;
	}

	case MODE_START_AT: {
		// try to get start planet number
		auto modeopt = std::string(argv[1]).substr(1);
		std::vector<std::string> keyValue = SplitString(modeopt, "=").to_vector<std::string>();

		// if found value
		if (keyValue.size() == 2) {
			if (keyValue[1].empty()) {
				startPath = SystemPath(0, 0, 0, 0, 0);
				Error("Please provide an actual SystemPath, like 0,0,0,0,18\n");
				return -1;
			} else {
				try {
					startPath = SystemPath::Parse(keyValue[1].c_str());
				} catch (const SystemPath::ParseFailure &spf) {
					startPath = SystemPath(0, 0, 0, 0, 0);
					Error("Failed to parse system path %s\n", keyValue[1].c_str());
					return -1;
				}
			}
		} else {
			// if value not exists - start on Sol, Mars, Cydonia
			startPath = SystemPath(0, 0, 0, 0, 18);
		}
		// set usual mode
		mode = MODE_GAME;
		// fallthrough
	}

	case MODE_GAME: {
		std::map<std::string, std::string> options;

		// if arguments more than parsed already
		if (argc > pos) {
			static const std::string delim("=");

			// for each argument
			for (; pos < argc; pos++) {
				const std::string arg(argv[pos]);
				std::vector<std::string> keyValue = SplitString(arg, "=").to_vector<std::string>();

				// if there no key and value || key is empty || value is empty
				if (keyValue.size() != 2 || keyValue[0].empty() || keyValue[1].empty()) {
					Output("malformed option: %s\n", arg.c_str());
					return 1;
				}

				// put key and value to config
				options[keyValue[0]] = keyValue[1];
			}
		}

		Pi::Init(options);

		if (startPath != SystemPath(0, 0, 0, 0, 0)) {
			Pi::GetApp()->SetStartPath(startPath);
		}

		Pi::GetApp()->Run();

		Pi::Uninit();
		break;
	}

	case MODE_VERSION: {
		std::string version(PIONEER_VERSION);
		if (strlen(PIONEER_EXTRAVERSION)) version += " (" PIONEER_EXTRAVERSION ")";
		Output("pioneer %s\n", version.c_str());
		OutputVersioningInfo();
		break;
	}

	case MODE_USAGE_ERROR:
		Output("pioneer: unknown mode %s\n", argv[1]);
		// fall through

	case MODE_USAGE:
		Output(
			"usage: pioneer [mode] [options...]\n"
			"available modes:\n"
			"    -game        [-g]     game (default)\n"
			"    -galaxydump  [-gd]    galaxy dumper\n"
			"    -startat     [-sa]    skip main menu and start at Mars\n"
			"    -startat=sp  [-sa=sp]  skip main menu and start at systempath x,y,z,si,bi\n"
			"    -version     [-v]     show version\n"
			"    -help        [-h,-?]  this help\n");
		break;
	}

	return 0;
}
