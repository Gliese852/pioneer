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
	return Engine.GetPerfString()
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
	s = s .. ", AI TARGET: " .. tostring(aTarget.label) .. "\nEST: " .. estimate_txt .. " DST: " .. altitude_txt .. " " .. altitude_unit


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
