local Engine = require 'Engine'
local ui = require 'pigui'
local utils = require 'utils'

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

local function dist(value)
	local value_txt, value_unit = ui.Format.DistanceUnit(value)
	return tostring(value_txt) .. " " .. tostring(value_unit)
end

local ftime = ui.Format.Duration

function Headless.FlightData()
	local Game = require 'Game'
	local player = Game.player

	local s = ""

	aTarget = Game.player:GetAutopilotTarget()
	if not aTarget then
		s = s .. ", NO AI TARGET";
	else
		local altitude = player:GetAltitudeRelTo(aTarget)
		local altitude_txt, altitude_unit = ui.Format.DistanceUnit(altitude)

		local estimate_txt = ui.Format.Duration(estimate(aTarget))
		s = s .. ", AI TARGET: " .. tostring(aTarget.label) .. "\nEST: " .. estimate_txt .. "  DST: " .. altitude_txt .. " " .. altitude_unit

		local velocity = player:GetVelocityRelTo(aTarget)
		local position = player:GetPositionRelTo(aTarget)
		local approach_speed = position:dot(velocity) / position:length()
		local speed, speed_unit = ui.Format.SpeedUnit(approach_speed)
		s = s .. "  APPROACH: " .. -speed .. " " .. speed_unit

		local brake_distance = player:GetDistanceToZeroV(velocity:length(), "forward")
		local distance, unit = ui.Format.DistanceUnit(brake_distance)
		s = s .. " BRA: " .. distance .. " " .. unit
	end

    local ShipDef = require 'ShipDef'
	local shipDef = ShipDef[player.shipId]
    local reserve = player:GetManualFuelReserve() * shipDef.fuelTankMass
    local avail = player.fuelMassLeft - reserve
    if avail < 0 then avail = 0 end

    s = s .. "\nFUEL: " .. string.format("%.1f / %.1f / %.1f t", avail, player.fuelMassLeft, shipDef.fuelTankMass )

    local thrust = player:GetThrusterState()
    s = s .. " -Z THRUST: " .. string.format("%.1f %%", -thrust.z * 100)

	local frame = player.frameBody

	if frame then
		local altitude = player:GetAltitudeRelTo(frame)
		s = s .. "\nFRAME: " .. frame.label .. " ALT: " .. dist(altitude)
	end

	local o = player:GetOrbit()
	s = s .. "\nORBIT: AP: " .. dist(o.apogeum) .. " PG: " .. dist(o.perigeum) .. " T: " .. ftime(o.period)

	return s
end

function Headless.HyperStatus()
	local Game = require 'Game'
	local path,destName = Game.player:GetHyperspaceDestination()
	local percent = Game.GetHyperspaceTravelledPercentage() * 100
	local due = Game.GetHyperspaceEndTime()

	if not path or not destName or not percent or not due then
		return 'NO DATA'
	end

	local s = "DEST: " .. destName .. " (" .. path.sectorX .. " " .. path.sectorY .. " " .. path.sectorZ .. ")"
  	local s = s .. " " .. string.format("%2.1f", percent) .. "%"
	local s = s .. "\nARRIVAL: " .. ui.Format.Date(due)
	local s = s .. " ETA: " ..  ui.Format.Duration(due - Game.time, 3)

	return s
end

function Headless.HyperRoute(n)
	local Game = require 'Game'
	local sectorView = Game.sectorView
	local route = sectorView:GetRoute()
	local start = Game.system and Game.system.path
	if not start then
		start = Game.InHyperspace() and Game.GetHyperspaceSource() or sectorView:GetCurrentSystemPath()
	end

	local player = Game.player

	local skippedDist = 0
	local skippedNum = 0
	local s = "SOURCE: " .. ui.Format.SystemPath(start) .. "\n"
	for i, path in pairs(route) do
		local status, distance, fuel, duration = player:GetHyperspaceDetails(start, path)
		if not n or n == #route or i == #route or i < n - 1 then
			s = s .. i .. ": " .. ui.Format.SystemPath(path) .. " " .. ui.Format.Number(distance, 2) .. " ly\n"
		else
			skippedDist = skippedDist + distance
			skippedNum = skippedNum + 1
			if i == #route - 1 then
				s = s .. "..." .. skippedNum .. " jumps " .. ui.Format.Number(skippedDist, 2) .. " ly\n"
			end
		end
		start = path
	end

	return s
end

function Headless.Jump(op)

	local Game = require 'Game'
	local player = Game.player

	local s = ""
	local allowJump = 2 -- 0 cant 1 legal 2 illegal

	local targetpath = player:GetHyperspaceTarget()
	s = s .. "\nTARGET: " .. ui.Format.SystemPath(targetpath)
	s = s .. "\nCAN JUMP: "

	if player:CanHyperjumpTo(targetpath) then
		if player:IsDocked() or player:IsLanded() then
			s = s .. "NO, LANDED/DOCKED"
		elseif player:IsHyperspaceActive() then
			s = s .. "NO, COUNTING"
		elseif not player:IsHyperjumpAllowed() then
			s = s .. "BETTER NO, NOT ALLOWED"
			allowJump = 1
		end
		s = s .. "YES, OK"
		allowJump = 0
	else
		s = s .. "NO, UNKNOWN REASON"
	end

	if op and op <= 2 and op > allowJump then
		s = s .. "\ninitiating jump"
		player:HyperjumpTo(player:GetHyperspaceTarget())
	else
		if not op then
			s = s .. "\nuse arg 1 to jump legal, arg 2 to to jump even illegal"
		else
			s = s .. "\ncan't jump with op " .. op
		end
	end

	return s
end

function Headless.Status()
	local Game = require 'Game'

	local year, month, day, hour, minute, second = Game.GetDateTime()
	local time = string.format("%04i-%02i-%02i - %02i:%02i:%02i", year, month, day, hour, minute, second)

	local s = "TIME: " .. time .. " PERF: " .. Headless.Perf()
	s = s .. "\nAI: " .. Game.player:GetAIStatusText()

	local fstate = Game.player.flightState
    s = s .. "\nSTATUS: " .. fstate;

	if fstate == 'FLYING' then
		s = s .. " " .. Headless.FlightData()
	elseif fstate == 'HYPERSPACE' then
		s = s .. " " .. Headless.HyperStatus()
		s = s .. "\n" ..Headless.HyperRoute(5)
	end

	return s
end

function Headless.Reload()
	return package.reimport('Headless')
end

return Headless
