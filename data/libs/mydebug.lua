-- often put in data/libs
local Vector2 = _G.Vector2
local dbg = {}

function dbg.cross(ui, pos, a_color, a_size)
	local color = a_color or ui.theme.colors.alertRed
	local size = a_size or 30.0
	ui.addLine(pos - Vector2(size, 0.0), pos + Vector2(size, 0.0), color, 1)
	ui.addLine(pos - Vector2(0.0, size), pos + Vector2(0.0, size), color, 1)
end


function dbg.paletteItem(ui, pos, color, text_arg)
	local size = 30
	local text = text_arg and text_arg ~= "" and text_arg or ""
	if color.a > 0 then text = text .. " " .. tostring(color) end
	ui.addRectFilled(pos, pos + Vector2(size,size), color, 0, 0)
	ui.addText(pos + Vector2(40, 8), ui.theme.colors.white, text)
	return pos + Vector2(0, size)
end

local n = 0
local log = {}
function dbg.logLine (text)
	n = n + 1
	table.insert(log, text)
end

function dbg.logVar(name, var)
	n = n + 1
	table.insert(log, name .. " = " .. tostring(var))
end

function dbg.drawLog(ui)
	for k, v in ipairs(log) do
		ui.addText(Vector2(10, 140 + 18 * k - 1), ui.theme.colors.white, tostring(v))
	end
	n = 0
	log = {}
end

local function displayTableWithFunc(dsp_fnc)
	local table_cache = {}
	local function genTable(name, tbl, lvl, indent, continue)
		if not continue then table_cache = {} end
		if not lvl then lvl = -1 end
		if not indent then
			indent = "...."
			dsp_fnc(name .. " = " .. tostring(tbl))
		end
		if lvl == 0 then return end
		if table_cache[tbl] then return end
		table_cache[tbl] = true
		for k,v in pairs(tbl) do
			local key = k and tostring(k) or "nil"
			local val = v and tostring(v) or "nil"
			dsp_fnc(indent .. "[" .. key .. "] = " .. val)
			if(type(v) == "table") then
				genTable("", v, lvl - 1, indent .. "....", true)
			end
		end
	end
	return genTable
end

dbg.printTable = displayTableWithFunc(print)
dbg.logTable = displayTableWithFunc(dbg.logLine)
dbg.displayTableWithFunc = displayTableWithFunc

function dbg.getnumber()
	return 0
end

function debug.logVar(name, val)
	dbg.logVar(name, val)
	return 0
end

--[[ CPP BRIDGE
usage in c++ side:
		auto l = Lua::manager->GetLuaState();
		lua_getglobal(l, "debug");
		LuaTable debug(l, -1);
		power = debug.Call<float>("calcpower", vector3d(m_torque / m_angThrust), vector3d(tu.pos), vector3d(tu.dir), id);
		lua_pop(l, 1);


LUA EXAMPLE
debug.calcpower = function(torq, pos, dir,id)
	return doSomeFloat()
end
--]]

-- print lua stack in gdb: print(pi_lua_stacktrace(l))

return dbg
