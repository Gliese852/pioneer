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

local pionillium = ui.fonts.pionillium
local ASTEROID_RADIUS = 1500000 -- rocky planets smaller than this (in meters) are considered an asteroid, not a planet

-- fake enum
local Projectable = { 
	-- types
	NONE = 0,
	PLAYER = 1,
	OBJECT = 2,
	L4 = 3,
	L5 = 4,
	APOAPSIS = 5,
	PERIAPSIS = 6,
	APSIS_GROUP = 7,
	LAGRANGE_GROUP = 8,
	-- reftypes
	BODY = 0,
	SYSTEMBODY = 1
}

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

local function getBodyIcon(obj)
	if obj.type == Projectable.APOAPSIS then return icons.apoapsis
	elseif obj.type == Projectable.PERIAPSIS then return icons.periapsis
	elseif obj.type == Projectable.L4 then return icons.lagrange
	elseif obj.type == Projectable.L5 then return icons.lagrange
	elseif obj.reftype == Projectable.SYSTEMBODY then
		local body = obj.ref
		local st = body.superType
		local t = body.type
		if st == "STARPORT" then
			if t == "STARPORT_ORBITAL" then
				return icons.spacestation
			elseif body.type == "STARPORT_SURFACE" then
				return icons.starport
			end
		elseif st == "GAS_GIANT" then
			return icons.gas_giant
		elseif st == "STAR" then
			return icons.sun
		elseif st == "ROCKY_PLANET" then
			if body.IsMoon then
				return icons.moon
			else
				if body.radius < ASTEROID_RADIUS then
					return icons.asteroid_hollow
				else
					return icons.rocky_planet
				end
			end
		end -- st
	else
		local body = obj.ref
		if body:IsShip() then
			local shipClass = body:GetShipClass()
			if icons[shipClass] then
				return icons[shipClass]
			else
				print("data/pigui/game.lua: getBodyIcon unknown ship class " .. (shipClass and shipClass or "nil"))
				return icons.ship -- TODO: better icon
			end
		elseif body:IsHyperspaceCloud() then
			return icons.hyperspace -- TODO: better icon
		elseif body:IsMissile() then
			return icons.bullseye -- TODO: better icon
		elseif body:IsCargoContainer() then
			return icons.rocky_planet -- TODO: better icon
		else
			print("data/pigui/game.lua: getBodyIcon not sure how to process body, supertype: " .. (st and st or "nil") .. ", type: " .. (t and t or "nil"))
			utils.print_r(body)
			return icons.ship
		end
	end
end

local function getLabel(obj)
	if obj.type == Projectable.OBJECT then
		if obj.reftype == Projectable.BODY then return obj.ref:GetLabel() else return obj.ref.name end
	elseif obj.type == Projectable.L4 and show_lagrange == "LAG_ICONTEXT" then return "L4"
	elseif obj.type == Projectable.L5 and show_lagrange == "LAG_ICONTEXT" then return "L5"
	else return ""
	end
end

local function getColor(obj)
	if obj.type == Projectable.OBJECT then return colors.frame end
	if obj.type == Projectable.APOAPSIS or obj.type == Projectable.PERIAPSIS then
		if obj.reftype == Projectable.BODY then return colors.blue end
		return colors.green
	end
	return colors.blue
end

local function displayOnScreenObjects()
	if ui.altHeld() and not ui.isAnyWindowHovered() and ui.isMouseClicked(1) then
		local frame = player.frameBody
		if frame then
			ui.openRadialMenu(frame, 1, 30, radial_menu_actions_orbital)
		end
	end
	local navTarget = player:GetNavTarget()
	local combatTarget = player:GetCombatTarget()

	ui.radialMenu("onscreenobjects")

	local should_show_label = ui.shouldShowLabels()
	local iconsize = Vector2(18 , 18)
	local label_offset = 14 -- enough so that the target rectangle fits
	local collapse = iconsize -- size of clusters to be collapsed into single bodies
	local click_radius = collapse:length() * 0.5
	-- make click_radius sufficiently smaller than the cluster size
	-- to prevent overlap of selection regions
	local objects_grouped = Engine.SystemMapGetProjectedGrouped(collapse, 1e64)

	for _,group in ipairs(objects_grouped) do
		local mainObject = group.mainObject
		local mainCoords = Vector2(group.screenCoordinates.x, group.screenCoordinates.y)

		-- indicators nav and combat target
		if mainObject.type == Projectable.OBJECT then
			if mainObject.reftype == Projectable.BODY and mainObject.ref == navTarget or mainObject.reftype == Projectable.SYSTEMBODY and navTarget and mainObject.ref == navTarget:GetSystemBody() then
				ui.addIcon(mainCoords, icons.square, colors.navTarget, indicatorSize, ui.anchor.center, ui.anchor.center)
			elseif mainObject.reftype == Projectable.BODY and mainObject.ref == combatTarget then
				ui.addIcon(mainCoords, icons.square, colors.combatTarget, indicatorSize, ui.anchor.center, ui.anchor.center)
			end
		end

		ui.addIcon(mainCoords, getBodyIcon(mainObject), getColor(mainObject), iconsize, ui.anchor.center, ui.anchor.center)

		if should_show_label then
			local label = getLabel(mainObject)
			if group.multiple then
				label = label .. " (" .. #group.objects .. ")"
			end
			ui.addStyledText(mainCoords + Vector2(label_offset,0), ui.anchor.left, ui.anchor.center, label , getColor(mainObject), pionillium.small)
		end
		local mp = ui.getMousePos()
		-- mouse release handler for right button
		if (mp - mainCoords):length() < click_radius then
			if not ui.isAnyWindowHovered() and ui.isMouseReleased(1) then
				ui.openPopup("navtarget" .. getLabel(mainObject))
			end
		end
		-- mouse release handler
		if (mp - mainCoords):length() < click_radius then
			if not ui.isAnyWindowHovered() and ui.isMouseReleased(0) and mainObject.type == Projectable.OBJECT then
				Engine.SystemMapCenterOn(mainObject.type, mainObject.reftype, mainObject.ref)
			end
		end
		ui.popup("navtarget" .. getLabel(mainObject), function()
			if ui.selectable("set as nav target", false, {}) then
				if mainObject.reftype == Projectable.BODY then
					player:SetNavTarget(mainObject.ref)
				elseif mainObject.ref.physicsBody then
					player:SetNavTarget(mainObject.ref.physicsBody)
				end
			end
		end)
	end
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

local function showTargetInfoWindow(obj)
	if obj.type == Projectable.NONE then return end
	ui.setNextWindowSize(Vector2(ui.screenWidth / 5, 0), "Always")
	ui.setNextWindowPos(Vector2(20, (ui.screenHeight / 5) * 2 + 20), "Always")
	ui.withStyleColors({["WindowBg"] = colors.lightBlackBackground}, function()
			ui.window("TargetInfoWindow", {"NoTitleBar", "AlwaysAutoResize", "NoResize", "NoFocusOnAppearing", "NoBringToFrontOnFocus", "NoSavedSettings"},
								function()
									local data
									if obj.type == Projectable.OBJECT and obj.reftype == Projectable.SYSTEMBODY then
										local systemBody = obj.ref
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
									elseif obj.type == Projectable.OBJECT and obj.reftype == Projectable.BODY and obj.ref:IsShip() then
										local body = obj.ref
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

local function displaySystemViewUI()
	player = Game.player
	local current_view = Game.CurrentView()

	if current_view == "system" and not Game.InHyperspace() then
			displayOnScreenObjects()
		ui.withFont(ui.fonts.pionillium.medium.name, ui.fonts.pionillium.medium.size, function()
			showOrbitPlannerWindow()
			showTargetInfoWindow(Engine.SystemMapSelectedObject())
		end)
	end
end

ui.registerModule("game", displaySystemViewUI)

return {}
