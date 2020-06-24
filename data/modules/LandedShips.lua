-- Copyright © 2008-2020 Pioneer Developers. See AUTHORS.txt for details
-- Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

local Engine = require 'Engine'
local Game = require 'Game'
local Space = require 'Space'
local Event = require 'Event'
local Serializer = require 'Serializer'
local ShipDef = require 'ShipDef'
local Ship = require 'Ship'
local utils = require 'utils'
local Timer = require 'Timer'

local loaded

doUndock = function (ship)
	-- the player may have left the system or the ship may have already undocked
	print "doUndock?"
	if ship:exists() and ship:GetDockedWith() then
		print ("try to undock from " .. ship:GetDockedWith().label)
		if not ship:Undock() then
			print "can't undock!"
			-- unable to undock, try again in ten minutes
			Timer:CallAt(Game.time + 60, function () doUndock(ship) end)
		else
			print "undock!"
		end
	end
end

local spawnShips = function ()
	local counter = 0
	local MIN_PERCENT = 100 
	local MAX_PERCENT = 100 
	local population = Game.system.population

	if population == 0 then
		return
	end

	local stations = Space.GetBodies(function (body) return body:isa("SpaceStation") end)
	if #stations == 0 then
		return
	end

	local shipdefs = utils.build_array(utils.filter(function (k,def) return def.tag == 'SHIP' end, pairs(ShipDef)))
	if #shipdefs == 0 then return end
	for _,station in pairs(stations) do
		local num_ships = math.floor(station.numDocks * Engine.rand:Integer(MIN_PERCENT, MAX_PERCENT) / 100)
		local need_undocker = false
		for i=1, num_ships do
			local ship = Space.SpawnShipDocked(shipdefs[Engine.rand:Integer(1,#shipdefs)].id, station)
			if ship then
				if need_undocker then
					Timer:CallAt(Game.time + 60, function () doUndock(ship) end)
					need_undocker = false
				end
				ship:SetLabel(Ship.MakeRandomLabel())
			end
			counter = counter + 1
		end
	end
	print ("Landed ships created: " .. counter)
end

local onEnterSystem = function (player)
	if not player:IsPlayer() then return end
	spawnShips()
end

local onGameStart = function ()
	if loaded == nil then
		spawnShips()
	end
	loaded = nil
end

local serialize = function ()
	return true
end

local unserialize = function (data)
	loaded = true
end

Event.Register("onEnterSystem", onEnterSystem)
Event.Register("onGameStart", onGameStart)

Serializer:Register("LandedShips", serialize, unserialize)
