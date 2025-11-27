-- Copyright © 2008-2025 Pioneer Developers. See AUTHORS.txt for details
-- Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

local ui = require 'pigui'
local gameView = require 'pigui.views.game'
local reticule = require 'pigui.modules.flight-ui.reticule'

local function draw()
	-- local velocity = player:GetVelocityRelTo(target)
	-- local position = player:GetPositionRelTo(target)
	-- local approach_speed = position:dot(velocity) / position:length()
	-- local altitude = player:GetAltitudeRelTo(target)
	-- local estimate = player:GetDurationForDistance(altitude, -approach_speed, 0.9)
	-- local estimate_txt = ui.Format.Duration(estimate)
	ui.text("RETICULE TARGET: " .. tostring(reticule.target))
	ui.text("ESTIMATE")
	ui.text("ARRIVAL TIME")
end

gameView.registerSidebarModule("autopilot", {
	side = "left",
	icon = ui.theme.icons.equip_autopilot,
	tooltip = "TOGGLE AUTPILOT", -- XXX json lui.TOGGLE_FULL_COMMS_WINDOW,
	title = "XAUTOPILOT", -- XXX json lc.COMMS,
	priority = 500,
	drawBody = draw,
})

return {}
