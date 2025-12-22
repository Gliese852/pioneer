local ui = require 'pigui.baseui'
local utils = require 'utils'
local debugView = require 'pigui.views.debug'
local Game = require 'Game'
local Space = require 'Space'
local SpaceStation = require 'SpaceStation'
local arrayTable = require 'pigui.libs.array-table'
local dbg = require 'mydebug'

local search_text = ""

debugView.registerTab('debug-adverts', {
	icon = ui.theme.icons.spacestation,
	show = function() return Game.system end,
	label = "Adverts",
	draw = function()
		local stations = utils.filter_array(Space.GetBodies("SpaceStation"), function (body) return true end)
		local ads = {}
		local filterFunction

		search_text, _ = ui.inputText("Search adverts", search_text, {})
		if #search_text > 0 then
			filterFunction = function(v) return string.match(v.description, search_text)
				or string.match(v.stationName, search_text)
			end
		end

		for _, station in pairs(stations) do
			if not SpaceStation.adverts[station] then
				SpaceStation.createStationData(station)
			end
			for _, v in pairs(SpaceStation.adverts[station]) do

				if not v.stationName then v.stationName = station.label end

				if not filterFunction or filterFunction(v) then
					table.insert(ads, v)
				end
			end
		end

		function formatDue(x)
			if x then return ui.Format.Duration(x - Game.time, 3) end
		end

		arrayTable.draw("all_stations_adverts", ads, ipairs, {
			{ name = "Station",     key = "stationName" },
			{ name = "Title",       key = "title" },
			{ name = "Location",    key = "location",   fnc = ui.Format.SystemPath, string = true },
			{ name = "Due",         key = "due",        fnc = formatDue },
			{ name = "Reward",      key = "reward" },
			{ name = "Description", key = "description" },
		},{ maxRows = 25 })
	end
})

