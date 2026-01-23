#include "Pi.h"
#include "lua/Lua.h"
#include "pigui/LuaPiGui.h"

#include "doctest.h"

TEST_CASE("ship_crash")
{
	std::map<std::string, std::string> options;
	Pi::Init(options, false);
	Lua::Init(Pi::GetAsyncJobQueue());
	PiGui::Lua::Init();
	Lua::InitModules();
}
