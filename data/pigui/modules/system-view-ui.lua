local Engine = import('Engine')
local Game = import('Game')
local gameView = import('pigui.views.game')
local ui = import('pigui/pigui.lua')
local Vector2 = _G.Vector2
local Lang = import("Lang")
local lc = Lang.GetResource("core")

local player = nil
local colors = ui.theme.colors
local icons = ui.theme.icons

local mainButtonSize = Vector2(32,32)  * (ui.screenHeight / 1200)
local mainButtonFramePadding = 3
local itemSpacing = Vector2(8, 4) -- couldn't get default from ui
local indicatorSize = Vector2(30 , 30)

local function showDvLine(leftIcon, resetIcon, rightIcon, key, Formatter, leftTooltip, resetTooltip, rightTooltip)
	local wheel = function()
		if ui.isItemHovered() then
			local w = ui.getMouseWheel()
			if w ~= 0 then
				Engine.TransferPlannerAdd(key, w * 10)
			end
		end
	end
	local press = ui.coloredSelectedIconButton(leftIcon, mainButtonSize, false, mainButtonFramePadding, colors.buttonBlue, colors.white, leftTooltip)
	if press or (key ~= "factor" and ui.isItemActive()) then
		Engine.TransferPlannerAdd(key, -10)
	end
	wheel()
	ui.sameLine()
	if ui.coloredSelectedIconButton(resetIcon, mainButtonSize, false, mainButtonFramePadding, colors.buttonBlue, colors.white, resetTooltip) then
		Engine.TransferPlannerReset(key)
	end
	wheel()
	ui.sameLine()
	press = ui.coloredSelectedIconButton(rightIcon, mainButtonSize, false, mainButtonFramePadding, colors.buttonBlue, colors.white, rightTooltip)
	if press or (key ~= "factor" and ui.isItemActive()) then
		Engine.TransferPlannerAdd(key, 10)
	end
	wheel()
	ui.sameLine()
	local speed, speed_unit = Formatter(Engine.TransferPlannerGet(key))
	ui.text(speed .. " " .. speed_unit)
	return 0
end

local time_selected_button_icon = icons.time_center

local function timeButton(icon, tooltip, factor)
	if ui.coloredSelectedIconButton(icon, mainButtonSize, false, mainButtonFramePadding, colors.buttonBlue, colors.white, tooltip) then
		time_selected_button_icon = icon
	end
	local active = ui.isItemActive()
	if active then
		Engine.SystemMapAccelerateTime(factor)
	end
	ui.sameLine()
	return active
end

local function VisibilityCircle(a, b, c) return { [a] = b, [b] = c, [c] = a } end

local ship_drawing = "SHIPS_OFF"
local show_lagrange = "LAG_OFF"
local show_grid = "GRID_OFF"
local nextShipDrawings = VisibilityCircle("SHIPS_OFF", "SHIPS_ON", "SHIPS_ORBITS")
local nextShowLagrange = VisibilityCircle("LAG_OFF", "LAG_ICON", "LAG_ICONTEXT")
local nextShowGrid = VisibilityCircle("GRID_OFF", "GRID_ON", "GRID_AND_LEGS")

local function calcWindowWidth(buttons)
	return (mainButtonSize.y + mainButtonFramePadding * 2) * buttons
	+ itemSpacing.x * (buttons + 1)
end

local function calcWindowHeight(buttons, separators, texts)
	return
	(mainButtonSize.y + mainButtonFramePadding * 2) * buttons
	+ separators * itemSpacing.y
	+ texts * ui.fonts.pionillium.medium.size
	+ (buttons + texts - 1) * itemSpacing.y
	+ itemSpacing.x * 2
end

local orbitPlannerWindowPos = Vector2(ui.screenWidth - calcWindowWidth(7), ui.screenHeight - calcWindowHeight(7, 3, 2))

local function showOrbitPlannerWindow()
	ui.setNextWindowPos(orbitPlannerWindowPos, "Always")
	ui.withStyleColors({["WindowBg"] = colors.lightBlackBackground}, function()
			ui.window("OrbitPlannerWindow", {"NoTitleBar", "NoResize", "NoFocusOnAppearing", "NoBringToFrontOnFocus", "NoSavedSettings", "AlwaysAutoResize"},
								function()
									ui.text("Orbit planner")

									ui.separator()

									if ui.coloredSelectedIconButton(icons.systemmap_reset_view, mainButtonSize, showShips, mainButtonFramePadding, colors.buttonBlue, colors.white, "Reset view") then
										Engine.SystemMapSetVisibility("RESET_VIEW")
									end
									ui.sameLine()
									if ui.coloredSelectedIconButton(icons.systemmap_toggle_grid, mainButtonSize, showShips, mainButtonFramePadding, colors.buttonBlue, colors.white, "Show grid") then
										show_grid = nextShowGrid[show_grid]
										Engine.SystemMapSetVisibility(show_grid);
									end
									ui.sameLine()
									if ui.coloredSelectedIconButton(icons.systemmap_toggle_ships, mainButtonSize, showShips, mainButtonFramePadding, colors.buttonBlue, colors.white, "Show ships") then
										ship_drawing = nextShipDrawings[ship_drawing]
										Engine.SystemMapSetVisibility(ship_drawing);
									end
									ui.sameLine()
									if ui.coloredSelectedIconButton(icons.systemmap_toggle_lagrange, mainButtonSize, showLagrangePoints, mainButtonFramePadding, colors.buttonBlue, colors.white, "Show Lagrange points") then
										show_lagrange = nextShowLagrange[show_lagrange]
										Engine.SystemMapSetVisibility(show_lagrange);
									end
									ui.sameLine()
									ui.coloredSelectedIconButton(icons.zoom_in,mainButtonSize, false, mainButtonFramePadding, colors.buttonBlue, colors.white, "Zoom in")
									if ui.isItemActive() then
										Engine.SystemMapSetVisibility("ZOOM_IN");
									end
									ui.sameLine()
									ui.coloredSelectedIconButton(icons.zoom_out, mainButtonSize, false, mainButtonFramePadding, colors.buttonBlue, colors.white, "Zoom out")
									if ui.isItemActive() then
										Engine.SystemMapSetVisibility("ZOOM_OUT");
									end

									ui.separator()

									showDvLine(icons.orbit_reduce, icons.orbit_delta, icons.orbit_increase, "factor", function(i) return i, "x" end, "Decrease delta factor", "Reset delta factor", "Increase delta factor")
									showDvLine(icons.orbit_reduce, icons.orbit_start_time, icons.orbit_increase, "starttime",
														 function(i)
															 local now = Game.time
															 local start = Engine.SystemMapGetOrbitPlannerStartTime()
															 if start then
																 return ui.Format.Duration(math.floor(start - now)), ""
															 else
																 return "now", ""
															 end
														 end,
														 "Decrease time", "Reset time", "Increase time")
									showDvLine(icons.orbit_reduce, icons.orbit_prograde, icons.orbit_increase, "prograde", ui.Format.Speed, "Thrust retrograde", "Reset prograde thrust", "Thrust prograde")
									showDvLine(icons.orbit_reduce, icons.orbit_normal, icons.orbit_increase, "normal", ui.Format.Speed, "Thrust antinormal", "Reset normal thrust", "Thrust normal")
									showDvLine(icons.orbit_reduce, icons.orbit_radial, icons.orbit_increase, "radial", ui.Format.Speed, "Thrust radially out", "Reset radial thrust", "Thrust radially in")

									ui.separator()

									local t = Engine.SystemMapGetOrbitPlannerTime()
									ui.text(t and ui.Format.Datetime(t) or "now")
									local r = false
									r = timeButton(icons.time_backward_100x, "-10,000,000x",-10000000) or r
									r = timeButton(icons.time_backward_10x, "-100,000x", -100000) or r
									r = timeButton(icons.time_backward_1x, "-1,000x", -1000) or r
									r = timeButton(icons.time_center, "Now", nil) or r
									r = timeButton(icons.time_forward_1x, "1,000x", 1000) or r
									r = timeButton(icons.time_forward_10x, "100,000x", 100000) or r
									r = timeButton(icons.time_forward_100x, "10,000,000x", 10000000) or r
									if not r then
										if time_selected_button_icon == icons.time_center then
											Engine.SystemMapAccelerateTime(nil)
										else
											Engine.SystemMapAccelerateTime(0.0)
										end
									end
			end)
	end)
end

local function tabular(data)
	if data and #data > 0 then
		ui.columns(2, "Attributes", true)
		for _,item in pairs(data) do
			if item.value then
				ui.text(item.name)
				ui.nextColumn()
				ui.text(item.value)
				ui.nextColumn()
			end
		end
		ui.columns(1, "NoAttributes", false)
	end
end

local function showTargetInfoWindow(body)
	local systemBody
	if body then systemBody = body:GetSystemBody() else return end
	ui.setNextWindowSize(Vector2(ui.screenWidth / 5, 0), "Always")
	ui.setNextWindowPos(Vector2(20, (ui.screenHeight / 5) * 2 + 20), "Always")
	ui.withStyleColors({["WindowBg"] = colors.lightBlackBackground}, function()
			ui.window("TargetInfoWindow", {"NoTitleBar", "AlwaysAutoResize", "NoResize", "NoFocusOnAppearing", "NoBringToFrontOnFocus", "NoSavedSettings"},
								function()
									local data
									if systemBody then
										local name = systemBody.name
										local rp = systemBody.rotationPeriod * 24 * 60 * 60
										local r = systemBody.radius
										local radius = nil
										if r and r > 0 then
											local v,u = ui.Format.Distance(r)
											radius = v .. u
										end
										local sma = systemBody.semiMajorAxis
										local semimajoraxis = nil
										if sma and sma > 0 then
											local v,u = ui.Format.Distance(sma)
											semimajoraxis = v .. u
										end
										local op = systemBody.orbitPeriod * 24 * 60 * 60
										data = {
											{ name = lc.NAME_OBJECT,
												value = name },
											{ name = lc.DAY_LENGTH .. lc.ROTATIONAL_PERIOD,
												value = rp > 0 and ui.Format.Duration(rp, 2) or nil },
											{ name = lc.RADIUS,
												value = radius },
											{ name = lc.SEMI_MAJOR_AXIS,
												value = semimajoraxis },
											{ name = lc.ORBITAL_PERIOD,
												value = op and op > 0 and ui.Format.Duration(op, 2) or nil }
										}
									elseif body and body:IsShip() then
										local name = body.label
										data = {{ name = lc.NAME_OBJECT,
															value = name },
										}
										-- TODO: the advanced target scanner should add additional data here,
										-- but we really do not want to hardcode that here. there should be
										-- some kind of hook that the target scanner can hook into to display
										-- more info here.
										-- This is what should be inserted:
										table.insert(data, { name = "Ship Type",
																				 value = body:GetShipType()
										})
										local hd = body:GetEquip("engine", 1)
										table.insert(data, { name = "Hyperdrive",
																				 value = hd and hd:GetName() or lc.NO_HYPERDRIVE
										})
										table.insert(data, { name = lc.MASS,
																				 value = ui.Format.MassT(body:GetStats().staticMass)
										})
										table.insert(data, { name = lc.CARGO,
																				 value = ui.Format.MassT(body:GetStats().usedCargo)
										})
									else
										data = {}
									end
									tabular(data)
			end)
	end)
end

local function showIndicators()
	local navTarget = player:GetNavTarget()
	local combatTarget = player:GetCombatTarget()
	for _,group in ipairs(gameView.bodies_grouped) do
		local mainBody = group.mainBody
		local mainCoords = group.screenCoordinates
		if mainBody == navTarget then
			ui.addIcon(mainCoords, icons.square, colors.navTarget, indicatorSize, ui.anchor.center, ui.anchor.center)
		elseif mainBody == combatTarget then
			ui.addIcon(mainCoords, icons.square, colors.combatTarget, indicatorSize, ui.anchor.center, ui.anchor.center)
		end
	end
end

local function displaySystemViewUI()
	player = Game.player
	local current_view = Game.CurrentView()

	if current_view == "system" and not Game.InHyperspace() then
		ui.withFont(ui.fonts.pionillium.medium.name, ui.fonts.pionillium.medium.size, function()
									showOrbitPlannerWindow()
									showIndicators()
									showTargetInfoWindow(Engine.SystemMapSelectedObject())
		end)
	end
end

ui.registerModule("game", displaySystemViewUI)

return {}
