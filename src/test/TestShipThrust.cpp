#include "Game.h"
#include "Space.h"
#include "FileSystem.h"
#include "lua/Lua.h"
#include "Pi.h"

#include "doctest.h"

TEST_CASE("ship_thrust")
{
	try {
		std::map<std::string, std::string> options;
		Pi::Init(options, true);
		Lua::Init(Pi::GetAsyncJobQueue());
		FileSystem::Init();
		SystemPath path{ 0, 0, 0, 1, 2 };
		Game g(path, 0);
	} catch (...) {
		Game *pg = nullptr;
		pg->IsNormalSpace();
	}
}
