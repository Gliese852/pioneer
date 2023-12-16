-- Copyright © 2008-2023 Pioneer Developers. See AUTHORS.txt for details
-- Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

local Equipment = require 'Equipment'

local EquipType = Equipment.EquipType
local LaserType = Equipment.LaserType

--===============================================
-- Pulse Cannons
--===============================================

Equipment.Register("laser.pulsecannon_1mw", LaserType.New {
	l10n_key="PULSECANNON_1MW",
	price=600, purchasable=true, tech_level=3,
	mass=2, volume=1.5, capabilities = {},
	slot = { type="weapon.laser", size=1, hardpoint=true },
	laser_stats = {
		lifespan=8, speed=1000, damage=1000, rechargeTime=0.25, length=30,
		width=5, beam=0, dual=0, mining=0, rgba_r = 255, rgba_g = 51, rgba_b = 51, rgba_a = 255
	},
	icon_name="equip_pulsecannon"
})

Equipment.Register("laser.pulsecannon_dual_1mw", LaserType.New {
	l10n_key="PULSECANNON_DUAL_1MW",
	price=1100, purchasable=true, tech_level=3,
	mass=4, volume=2, capabilities = {},
	slot = { type="weapon.laser", size=2, hardpoint=true },
	laser_stats = {
		lifespan=8, speed=1000, damage=1000, rechargeTime=0.25, length=30,
		width=5, beam=0, dual=1, mining=0, rgba_r = 255, rgba_g = 51, rgba_b = 51, rgba_a = 255
	},
	icon_name="equip_pulsecannon"
})

Equipment.Register("laser.pulsecannon_2mw", LaserType.New {
	l10n_key="PULSECANNON_2MW",
	price=1000, purchasable=true, tech_level=5,
	mass=3, volume=2.5, capabilities = {},
	slot = { type="weapon.laser", size=2, hardpoint=true },
	laser_stats = {
		lifespan=8, speed=1000, damage=2000, rechargeTime=0.25, length=30,
		width=5, beam=0, dual=0, mining=0, rgba_r = 255, rgba_g = 127, rgba_b = 51, rgba_a = 255
	},
	icon_name="equip_pulsecannon"
})

Equipment.Register("laser.pulsecannon_rapid_2mw", LaserType.New {
	l10n_key="PULSECANNON_RAPID_2MW",
	price=1800, purchasable=true, tech_level=5,
	mass=7, volume=7, capabilities={},
	slot = { type="weapon.laser", size=2, hardpoint=true },
	laser_stats = {
		lifespan=8, speed=1000, damage=2000, rechargeTime=0.13, length=30,
		width=5, beam=0, dual=0, mining=0, rgba_r = 255, rgba_g = 127, rgba_b = 51, rgba_a = 255
	},
	icon_name="equip_pulsecannon_rapid"
})

Equipment.Register("laser.pulsecannon_4mw", LaserType.New {
	l10n_key="PULSECANNON_4MW",
	price=2200, purchasable=true, tech_level=6,
	mass=10, volume=10, capabilities={},
	slot = { type="weapon.laser", size=3, hardpoint=true },
	laser_stats = {
		lifespan=8, speed=1000, damage=4000, rechargeTime=0.25, length=30,
		width=5, beam=0, dual=0, mining=0, rgba_r = 255, rgba_g = 255, rgba_b = 51, rgba_a = 255
	},
	icon_name="equip_pulsecannon"
})

Equipment.Register("laser.pulsecannon_10mw", LaserType.New {
	l10n_key="PULSECANNON_10MW",
	price=4900, purchasable=true, tech_level=7,
	mass=30, volume=30, capabilities={},
	slot = { type="weapon.laser", size=4, hardpoint=true },
	laser_stats = {
		lifespan=8, speed=1000, damage=10000, rechargeTime=0.25, length=30,
		width=5, beam=0, dual=0, mining=0, rgba_r = 51, rgba_g = 255, rgba_b = 51, rgba_a = 255
	},
	icon_name="equip_pulsecannon"
})

Equipment.Register("laser.pulsecannon_20mw", LaserType.New {
	l10n_key="PULSECANNON_20MW",
	price=12000, purchasable=true, tech_level="MILITARY",
	mass=65, volume=65, capabilities={},
	slot = { type="weapon.laser", size=5, hardpoint=true },
	laser_stats = {
		lifespan=8, speed=1000, damage=20000, rechargeTime=0.25, length=30,
		width=5, beam=0, dual=0, mining=0, rgba_r = 0.1, rgba_g = 51, rgba_b = 255, rgba_a = 255
	},
	icon_name="equip_pulsecannon"
})

--===============================================
-- Beam Lasers
--===============================================

Equipment.Register("laser.beamlaser_1mw", LaserType.New {
	l10n_key="BEAMLASER_1MW",
	price=2400, purchasable=true, tech_level=4,
	mass=3, volume=3, capabilities={},
	slot = { type="weapon.laser", size=1, hardpoint=true },
	laser_stats = {
		lifespan=8, speed=1000, damage=1500, rechargeTime=0.25, length=10000,
		width=1, beam=1, dual=0, mining=0, rgba_r = 255, rgba_g = 51, rgba_b = 127, rgba_a = 255,
		heatrate=0.02, coolrate=0.01
	},
	icon_name="equip_beamlaser"
})

Equipment.Register("laser.beamlaser_dual_1mw", LaserType.New {
	l10n_key="BEAMLASER_DUAL_1MW",
	price=4800, purchasable=true, tech_level=5,
	mass=6, volume=6, capabilities={},
	slot = { type="weapon.laser", size=2, hardpoint=true },
	laser_stats = {
		lifespan=8, speed=1000, damage=1500, rechargeTime=0.5, length=10000,
		width=1, beam=1, dual=1, mining=0, rgba_r = 255, rgba_g = 51, rgba_b = 127, rgba_a = 255,
		heatrate=0.02, coolrate=0.01
	},
	icon_name="equip_dual_beamlaser"
})

Equipment.Register("laser.beamlaser_2mw", LaserType.New {
	l10n_key="BEAMLASER_RAPID_2MW",
	price=5600, purchasable=true, tech_level=6,
	mass=7, volume=7, capabilities={},
	slot = { type="weapon.laser", size=2, hardpoint=true },
	laser_stats = {
		lifespan=8, speed=1000, damage=3000, rechargeTime=0.13, length=20000,
		width=1, beam=1, dual=0, mining=0, rgba_r = 255, rgba_g = 192, rgba_b = 192, rgba_a = 255,
		heatrate=0.02, coolrate=0.01
	},
	icon_name="equip_beamlaser"
})

--===============================================
-- Plasma Accelerators
--===============================================

Equipment.Register("laser.small_plasma_accelerator", LaserType.New {
	l10n_key="SMALL_PLASMA_ACCEL",
	price=120000, purchasable=true, tech_level=10,
	mass=22, volume=22, capabilities={},
	slot = { type="weapon.laser", size=5, hardpoint=true },
	laser_stats = {
		lifespan=8, speed=1000, damage=50000, rechargeTime=0.3, length=42,
		width=7, beam=0, dual=0, mining=0, rgba_r = 51, rgba_g = 255, rgba_b = 255, rgba_a = 255
	},
	icon_name="equip_plasma_accelerator"
})

Equipment.Register("laser.large_plasma_accelerator", LaserType.New {
	l10n_key="LARGE_PLASMA_ACCEL",
	price=390000, purchasable=true, tech_level=12,
	mass=50, volume=50, capabilities={},
	slot = { type="weapon.laser", size=6, hardpoint=true },
	laser_stats = {
		lifespan=8, speed=1000, damage=100000, rechargeTime=0.3, length=42,
		width=7, beam=0, dual=0, mining=0, rgba_r = 127, rgba_g = 255, rgba_b = 255, rgba_a = 255
	},
	icon_name="equip_plasma_accelerator"
})

--===============================================
-- Mining Cannons
--===============================================

Equipment.Register("laser.miningcannon_5mw", LaserType.New {
	l10n_key="MININGCANNON_5MW",
	price=3700, purchasable=true, tech_level=5,
	mass=6, volume=6, capabilities={},
	slot = { type="weapon.mining", size=2, hardpoint=true },
	laser_stats = {
		lifespan=8, speed=1000, damage=5000, rechargeTime=1.5, length=30,
		width=5, beam=0, dual=0, mining=1, rgba_r = 51, rgba_g = 127, rgba_b = 0, rgba_a = 255
	},
	icon_name="equip_mining_laser"
})

Equipment.Register("laser.miningcannon_17mw", LaserType.New {
	l10n_key="MININGCANNON_17MW",
	price=10600, purchasable=true, tech_level=8,
	mass=10, volume=10, capabilities={},
	slot = { type="weapon.mining", size=4, hardpoint=true },
	laser_stats = {
		lifespan=8, speed=1000, damage=17000, rechargeTime=2, length=30,
		width=5, beam=0, dual=0, mining=1, rgba_r = 51, rgba_g = 127, rgba_b = 0, rgba_a = 255
	},
	icon_name="equip_mining_laser"
})

--===============================================
-- Missiles
--===============================================

Equipment.Register("missile.unguided_s1", EquipType.New {
	l10n_key="MISSILE_UNGUIDED",
	price=30, purchasable=true, tech_level=1,
	missile_type="missile_unguided",
	volume=0, mass=0.1,
	slot = { type="missile", size=1, hardpoint=true },
	icon_name="equip_missile_unguided"
})
Equipment.Register("missile.guided_s2", EquipType.New {
	l10n_key="MISSILE_GUIDED",
	price=50, purchasable=true, tech_level=5,
	missile_type="missile_guided",
	volume=0, mass=0.3,
	slot = { type="missile", size=2, hardpoint=true },
	icon_name="equip_missile_guided"
})
Equipment.Register("missile.smart_s3", EquipType.New {
	l10n_key="MISSILE_SMART",
	price=95, purchasable=true, tech_level=10,
	missile_type="missile_smart",
	volume=0, mass=0.5,
	slot = { type="missile", size=3, hardpoint=true },
	icon_name="equip_missile_smart"
})
Equipment.Register("missile.naval_s4", EquipType.New {
	l10n_key="MISSILE_NAVAL",
	price=160, purchasable=true, tech_level="MILITARY",
	missile_type="missile_naval",
	volume=0, mass=1,
	slot = { type="missile", size=4, hardpoint=true },
	icon_name="equip_missile_naval"
})
