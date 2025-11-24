-- Copyright © 2008-2025 Pioneer Developers. See AUTHORS.txt for details
-- Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

local ui = require 'pigui'
local Game = require 'Game'
local gameView = require 'pigui.views.game'
local reticuleID = gameView.modules.reticule
local reticule = reticuleID and gameView.modules[reticuleID]

local function reticuleTarget()
	local name = reticule.target()
	if name == 'combatTarget' then
		return Game.player:GetCombatTarget()
	elseif name == 'navTarget' then
		return Game.player:GetNavTarget()
	end
end

local function estimate(target)
	local player = Game.player
	local velocity = player:GetVelocityRelTo(target)
	local position = player:GetPositionRelTo(target)
	local approach_speed = position:dot(velocity) / position:length()
	local altitude = player:GetAltitudeRelTo(target)
	local estimate = player:GetDurationForDistance(altitude, -approach_speed, 0.9)
	return estimate
end

local function drawEstimate(target)
	if (not target) then
		ui.text("NO TARGET")
		return
	end
	local estimate_txt = ui.Format.Duration(estimate(target))
	ui.text("NAME: " .. tostring(target.label))
	ui.text("EST: " .. estimate_txt)
end

local function draw()
	local rTarget = reticuleTarget()
	local aTarget = Game.player:GetAutopilotTarget()
	-- local velocity = player:GetVelocityRelTo(target)
	-- local position = player:GetPositionRelTo(target)
	-- local approach_speed = position:dot(velocity) / position:length()
	-- local altitude = player:GetAltitudeRelTo(target)
	-- local estimate = player:GetDurationForDistance(altitude, -approach_speed, 0.9)
	-- local estimate_txt = ui.Format.Duration(estimate)
	-- ui.text("RETICULE TARGET: " .. tostring(reticuleTarget))
	-- ui.text("AUTOPILOT TARGET: " .. tostring(autopilotTarget))

	local fuelReserve = Game.player:GetManualFuelReserve()
	local newFuelReserve = ui.dragFloat("Fuel Re", fuelReserve, 0.05, 0.0, 1.0, "%f")

	if newFuelReserve ~= fuelReserve then
		Game.player:SetManualFuelReserve(newFuelReserve)
	end

	if rTarget ~= aTarget then
		ui.text("RETICULE TARGET:")
		drawEstimate(rTarget)
	end

	ui.text("AUTOPILOT TARGET:")
	drawEstimate(aTarget)

	ui.text("MANUAL LENGTH:")
	local text = "0.1"
	local changed = false
	text, changed = ui.inputText("##manual_length", text, {})
	ui.sameLine()
	ui.text("AU")
	local len = tonumber(text)

	if len then
		local AU = 149598000000
		local estimate = Game.player:GetDurationForDistance(len * AU, 0.0, 0.9)
		local estimate_txt = ui.Format.Duration(estimate)
		ui.text("EST: " .. estimate_txt)
	else
		ui.text("WRONG NUMBER")
	end
end

gameView.registerSidebarModule("autopilot", {
	side = "left",
	icon = ui.theme.icons.equip_autopilot,
	tooltip = "TOGGLE AUTPILOT", -- XXX json lui.TOGGLE_FULL_COMMS_WINDOW,
	title = "AUTOPILOT", -- XXX json lc.COMMS,
	priority = 500,
	drawBody = draw,
	debugReload = function() package.reimport() end,
})

return {}
