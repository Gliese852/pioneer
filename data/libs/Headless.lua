local Engine = require 'Engine'
local ui = require 'pigui'

local Headless = {}

function Headless.StartGame()
	SystemPath = require 'SystemPath'
	Game = require 'Game'
	location = SystemPath.New(0,0,0,0,18)
	startTime = util.standardGameStartTime()
	shipType = 'pumpkinseed'
	Game.StartGame(location, startTime, shipType)
	Game.SetRealtimeMode(true)
end

function Headless.LoadGame(filename)
	local Game = require 'Game'
	Game.LoadGame(filename)
	Game.SetRealtimeMode(true)
end

function Headless.SaveGame(filename)
	local Game = require 'Game'
	Game.SaveGame(filename)
end

function Headless.EndGame()
	local Game = require 'Game'
	Game.EndGame()
end

function Headless.Quit()
	Engine.Quit()
end

function Headless.Perf()
	local stat = Engine.GetPerfStat()
    return string.format("fps %.1f phys %.1f %s mem %.1f %s", 1000 / stat.fps.recent, stat.phys.recent, stat.phys.unit, stat.procmem.recent, stat.procmem.unit)
end

function Headless.Profile()
	return Engine.RequestProfileFrame()
end

local function estimate(target)
	local Game = require 'Game'
	local player = Game.player
	local velocity = player:GetVelocityRelTo(target)
	local position = player:GetPositionRelTo(target)
	local approach_speed = position:dot(velocity) / position:length()
	local altitude = player:GetAltitudeRelTo(target)
	local estimate = player:GetDurationForDistance(altitude, -approach_speed, 0.9)
	return estimate
end

function Headless.FlightData()
	local Game = require 'Game'
	local player = Game.player
	local fstate = player.flightState
	local s = ""
	if fstate ~= 'FLYING' then
		return "STATUS: " .. fstate
	else
		s = "STATUS: FLYING"
	end

	local s = ""

	aTarget = Game.player:GetAutopilotTarget()
	if not aTarget then
		s = s .. ", NO AI TARGET";
		return s;
	end

	local altitude = player:GetAltitudeRelTo(aTarget)
	local altitude_txt, altitude_unit = ui.Format.DistanceUnit(altitude)

	local estimate_txt = ui.Format.Duration(estimate(aTarget))
	s = s .. ", AI TARGET: " .. tostring(aTarget.label) .. "\nEST: " .. estimate_txt .. "  DST: " .. altitude_txt .. " " .. altitude_unit

	local velocity = player:GetVelocityRelTo(aTarget)
	local position = player:GetPositionRelTo(aTarget)
	local approach_speed = position:dot(velocity) / position:length()
	local speed, speed_unit = ui.Format.SpeedUnit(approach_speed)
    s = s .. "  APPROACH: " .. -speed .. " " .. speed_unit

    local ShipDef = require 'ShipDef'
	local shipDef = ShipDef[player.shipId]
    local reserve = player:GetManualFuelReserve() * shipDef.fuelTankMass
    local avail = player.fuelMassLeft - reserve
    if avail < 0 then avail = 0 end

    s = s .. "\nFUEL: " .. string.format("%.1f / %.1f / %.1f t", avail, player.fuelMassLeft, shipDef.fuelTankMass )

    local thrust = player:GetThrusterState()
    -- s = s .. " THRUST: " .. tostring(-thrust.z)
    s = s .. " -Z THRUST: " .. string.format("%.1f %%", -thrust.z)

	return s
end

function Headless.Status()
	local Game = require 'Game'

	local year, month, day, hour, minute, second = Game.GetDateTime()
	local time = string.format("%04i-%02i-%02i - %02i:%02i:%02i", year, month, day, hour, minute, second)

	local s = "TIME: " .. time .. " PERF: " .. Headless.Perf()
	s = s .. "\nAI: " .. Game.player:GetAIStatusText()
	s = s .. "\n" .. Headless.FlightData()

	return s
end

function Headless.Reload()
	return package.reimport('Headless')
end

return Headless
