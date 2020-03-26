-- Copyright © 2008-2020 Pioneer Developers. See AUTHORS.txt for details
-- Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

local theme = {}

theme.colors = {
	reticuleCircle = Color(200, 200, 200),
	reticuleCircleDark = Color(150, 150, 150),
	frame = Color(200, 200, 200),
	frameDark = Color(120, 120, 120),
	transparent = Color(0, 0, 0, 0),
	navTarget = Color(237, 237, 112),
	navTargetDark = Color(160, 160, 50),
	combatTarget = Color(237, 112, 112),
	combatTargetDark = Color(160, 50, 50),
	-- navTarget = Color(0, 255, 0),
	-- navTargetDark = Color(0, 150, 0),
	navigationalElements = Color(200, 200, 200),
	deltaVCurrent = Color(150, 150, 150),
	deltaVManeuver = Color(168, 168, 255),
	deltaVRemaining = Color(250, 250, 250),
	deltaVTotal = Color(100, 100, 100, 200),
	brakeBackground = Color(100, 100, 100, 200),
	brakePrimary = Color(200, 200, 200),
	brakeSecondary = Color(120, 120, 120),
	brakeNow = Color(150, 200, 150),
	brakeOvershoot = Color(184, 10, 10),
	maneuver = Color(200, 150, 200),
	maneuverDark = Color(160, 50, 160),
	mouseMovementDirection = Color(160, 160, 50),
	lightBlueBackground = Color(0, 0, 200, 20),
	blueBackground = Color(0, 0, 225, 50),
	blueFrame = Color(190, 225, 255, 100),
	commsWindowBackground = Color(20, 20, 80, 200),
	lightBlackBackground = Color(0, 0, 0, 100),
	buttonBlue = Color(150, 150, 200, 255),
	white = Color(255,255,255,255),
	grey = Color(120,120,120,255),
	lightGrey = Color(200,200,200,255),
	gaugeBackground = Color(40, 40, 70),
	gaugePressure = Color(76,76,158),
	gaugeTemperature = Color(200,0,0),
	gaugeShield = Color(150,150,230),
	gaugeHull = Color(230,230,230),
	gaugeWeapon = Color(255,165, 0),
	alertYellow = Color(212,182,34),
	alertRed = Color(165,40,40),
	black = Color(0,0,0),
	hyperspaceInfo = Color(80, 200, 80),
	gaugeVelocityLight = Color(230,230,230),
	gaugeVelocityDark = Color(30,30,30),
	gaugeThrustLight = Color(123,123,123),
	gaugeThrustDark = Color(19, 19, 31),
	radarCombatTarget = Color(255, 0, 0),
	radarShip = Color(243, 237, 29),
	radarPlayerMissile = Color(243, 237, 29),
	radarMissile = Color(240, 38, 50),
	radarCargo = Color(166, 166, 166),
	radarNavTarget = Color(0, 255, 0),
	radarStation = Color(255, 255, 255),
	radarCloud = Color(128, 128, 255),
    gaugeEquipmentMarket = Color(76,76,158),
}

theme.icons = {
   -- first row
   prograde = 0,
   retrograde = 1,
   radial_out = 2,
   radial_in = 3,
   antinormal = 4,
   normal = 5,
   frame = 6,
   maneuver = 7,
   forward = 8,
   backward = 9,
   down = 10,
   right = 11,
   up = 12,
   left = 13,
   bullseye = 14,
   square = 15,
   -- second row
   prograde_thin = 16,
   retrograde_thin = 17,
   radial_out_thin = 18,
   radial_in_thin = 19,
   antinormal_thin = 20,
   normal_thin = 21,
   frame_away = 22,
   direction = 24,
   direction_hollow = 25,
   direction_frame = 26,
   direction_frame_hollow = 27,
   direction_forward = 28,
   semi_major_axis = 31,
   -- third row
   heavy_fighter = 32,
   medium_fighter = 33,
   light_fighter = 34,
   sun = 35,
   asteroid_hollow = 36,
   current_height = 37,
   current_periapsis = 38,
   current_line = 39,
   current_apoapsis = 40,
   eta = 41,
   altitude = 42,
   gravity = 43,
   eccentricity = 44,
   inclination = 45,
   longitude = 46,
   latitude = 47,
   -- fourth row
   heavy_courier = 48,
   medium_courier = 49,
   light_courier = 50,
   rocky_planet = 51,
   ship = 52, -- useless?
   landing_gear_up = 53,
   landing_gear_down = 54,
   ecm = 55,
   rotation_damping_on = 56,
   rotation_damping_off = 57,
   hyperspace = 58,
   hyperspace_off = 59,
   scanner = 60,
   message_bubble = 61,
   fuel = 63,
   -- fifth row
   heavy_passenger_shuttle = 64,
   medium_passenger_shuttle = 65,
   light_passenger_shuttle = 66,
   moon = 67,
   autopilot_set_speed = 68,
   autopilot_manual = 69,
   autopilot_fly_to = 70,
   autopilot_dock = 71,
   autopilot_hold = 72,
   autopilot_undock = 73,
   autopilot_undock_illegal = 74,
   autopilot_blastoff = 75,
   autopilot_blastoff_illegal = 76,
   autopilot_low_orbit = 77,
   autopilot_medium_orbit = 78,
   autopilot_high_orbit = 79,
   -- sixth row
   heavy_passenger_transport = 80,
   medium_passenger_transport = 81,
   light_passenger_transport = 82,
   gas_giant = 83,
   time_accel_stop = 84,
   time_accel_paused = 85,
   time_accel_1x = 86,
   time_accel_10x = 87,
   time_accel_100x = 88,
   time_accel_1000x = 89,
   time_accel_10000x = 90,
   pressure = 91,
   shield = 92,
   hull = 93,
   temperature = 94,
   rotate_view = 95,
   -- seventh row
   heavy_cargo_shuttle = 96,
   medium_cargo_shuttle = 97,
   light_cargo_shuttle = 98,
   spacestation = 99,
   time_backward_100x = 100,
   time_backward_10x = 101,
   time_backward_1x = 102,
   time_center = 103,
   time_forward_1x = 104,
   time_forward_10x = 105,
   time_forward_100x = 106,
   filter_bodies = 107,
   filter_stations = 108,
   filter_ships = 109,
   lagrange_marker = 110,
   system_overview = 111,
   -- eighth row
   heavy_freighter = 112,
   medium_freighter = 113,
   light_freighter = 114,
   starport = 115,
   warning_1 = 116,
   warning_2 = 117,
   warning_3 = 118,
   display_frame = 119,
   display_combattarget = 120,
   display_navtarget = 121,
   alert1 = 122,
   alert2 = 123,
   ecm_advanced = 124,
   systems_management = 125,
   distance = 126,
   filter = 127,
   -- ninth row
   view_internal = 128,
   view_external = 129,
   view_sidereal = 130,
   comms = 131,
   market = 132,
   bbs = 133,
   equipment = 134,
   repairs = 135,
   info = 136,
   personal_info = 137,
   personal = 138,
   rooster = 139,
   map = 140,
   sector_map = 141,
   system_map = 142,
   system_overview = 143,
   -- tenth row
   galaxy_map = 144,
   settings = 145,
   language = 146,
   controls = 147,
   sound = 148,
   new = 149,
   skull = 150,
   mute = 151,
   unmute = 152,
   music = 153,
   zoom_in = 154,
   zoom_out = 155,
   search_lens = 156,
   message = 157,
   message_open = 158,
   search_binoculars = 159,
   -- eleventh row
   planet_grid = 160,
   bookmarks = 161,
   unlocked = 162,
   locked = 163,
   label = 165,
   broadcast = 166,
   shield_other = 167,
   hud = 168,
   factory = 169,
   star = 170,
   delta = 171,
   clock = 172,
   orbit_prograde = 173,
   orbit_normal = 174,
   orbit_radial = 175,
   -- twelfth row
   view_flyby = 191,
   -- thirteenth row
   cog = 192,
   gender = 193,
   nose = 194,
   mouth = 195,
   hair = 196,
   clothes = 197,
   accessories = 198,
   random = 199,
   periapsis = 200,
   apoapsis = 201,
   reset_view = 202,
   toggle_grid = 203,
   toggle_lagrange = 204,
   toggle_ships = 205,
   decrease = 206,
   increase = 207,
   -- fourteenth row, wide icons
   missile_unguided = 208,
   missile_guided = 210,
   missile_smart = 212,
   missile_naval = 214,
   find_person = 216,
   cargo_manifest = 217,
   trashcan = 218,
   bookmark = 219,
   pencil = 220,
   fountain_pen = 221,
   cocktail_glass = 222,
   beer_glass = 223,
   -- fifteenth row
   chart = 224,
   binder = 225,
   navtarget = 226,
   -- TODO: manual / autopilot
   -- dummy, until actually defined correctly
   mouse_move_direction = 14,
}

-- TODO: apply these styles at startup.
theme.styles = {
	WindowBorderSize = 0.0,
	FrameBorderSize = 0.0
}

return theme
