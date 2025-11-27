-- Copyright © 2008-2025 Pioneer Developers. See AUTHORS.txt for details
-- Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

local ui = require 'pigui'
local gameView = require 'pigui.views.game'

local function draw()
	ui.text("HELLO, AUTOPILOT")
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
