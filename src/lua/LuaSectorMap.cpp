// Copyright © 2008-2022 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "Game.h"
#include "LuaColor.h"
#include "LuaMetaType.h"
#include "LuaObject.h"
#include "LuaVector.h"
#include "SectorMap.h"

template <>
const char *LuaObject<SectorMap>::s_type = "SectorMap";

template <>
void LuaObject<SectorMap>::RegisterClass()
{
	static LuaMetaType<SectorMap> metaType(s_type);
	metaType.CreateMetaType(Lua::manager->GetLuaState());
	metaType.StartRecording()
		.AddFunction("GetZoomLevel", &SectorMap::GetZoomLevel)
		.AddFunction("ZoomIn", &SectorMap::ZoomIn)
		.AddFunction("ZoomOut", &SectorMap::ZoomOut)
		.AddFunction("SetDrawVerticalLines", &SectorMap::SetDrawVerticalLines)
		.AddFunction("SetFactionVisible", &SectorMap::SetFactionVisible)
		.AddFunction("SetDrawUninhabitedLabels", &SectorMap::SetDrawUninhabitedLabels)
		.AddFunction("GotoSectorPath", &SectorMap::GotoSector)
		.AddFunction("GotoSystemPath", &SectorMap::GotoSystem)
		.AddFunction("SetRotateMode", &SectorMap::SetRotateMode)
		.AddFunction("SetZoomMode", &SectorMap::SetZoomMode)
		.AddFunction("ResetView", &SectorMap::ResetView)
		.AddFunction("IsCenteredOn", &SectorMap::IsCenteredOn)
		.AddFunction("SetLabelParams", &SectorMap::SetLabelParams)
		.AddFunction("SetLabelsVisibility", &SectorMap::SetLabelsVisibility)
		.AddFunction("GetFactions", [](lua_State *l, SectorMap *sm) -> int {
			const std::set<const Faction *> visible = sm->GetVisibleFactions();
			const std::set<const Faction *> hidden = sm->GetHiddenFactions();
			lua_newtable(l); // outer table
			int i = 1;
			for (const Faction *f : visible) {
				lua_pushnumber(l, i++);
				lua_newtable(l); // inner table
				LuaObject<Faction>::PushToLua(const_cast<Faction *>(f));
				lua_setfield(l, -2, "faction");
				lua_pushboolean(l, hidden.count(f) == 0);
				lua_setfield(l, -2, "visible"); // inner table
				lua_settable(l, -3);			// outer table
			}
			return 1;
		})
		.AddFunction("SearchNearbyStarSystemsByName", [](lua_State *l, SectorMap *sm) -> int {
			std::string pattern = LuaPull<std::string>(l, 2);
			std::vector<SystemPath> matches = sm->GetNearbyStarSystemsByName(pattern);
			int i = 1;
			lua_newtable(l);
			for (const SystemPath &path : matches) {
				lua_pushnumber(l, i++);
				LuaObject<SystemPath>::PushToLua(path);
				lua_settable(l, -3);
			}
			return 1;
		})
		.StopRecording();
	LuaObjectBase::CreateClass(&metaType);
}
