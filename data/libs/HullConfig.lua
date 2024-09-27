-- Copyright © 2008-2024 Pioneer Developers. See AUTHORS.txt for details
-- Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

local ShipDef = require 'ShipDef'
local Serializer = require 'Serializer'

local utils = require 'utils'

-- Class: HullConfig.Slot
--
-- Generic interface for an equipment-containing slot in a shipdef
--
-- Represents a constrained potential mounting point for ship equipment
-- either internal or external to a ship.
-- Can contain additional fields for specific slot types.
---@class HullConfig.Slot
---@field clone fun(self, mixin):self
local Slot = utils.proto("HullConfig.Slot")

Slot.id = ""
Slot.type = ""
Slot.size = 1
Slot.size_min = nil ---@type number?
Slot.tag = nil ---@type string?
Slot.default = nil ---@type string?
Slot.hardpoint = false
Slot.i18n_key = nil ---@type string?
Slot.i18n_res = "equipment-core"
Slot.count = nil ---@type integer?

-- Class: HullConfig
--
-- Represents a specific "ship configuration", being a list of equipment slots
-- and associated data.
--
-- The default configuration for a ship hull is defined in its JSON shipdef and
-- consumed by Lua as a ship config.
---@class HullConfig
---@field clone fun():self
local HullConfig = utils.proto("HullConfig")

HullConfig.id = ""
HullConfig.capacity = 0

-- Default slot config for a new shipdef
-- Individual shipdefs can redefine slots or remove them by setting the slot to 'false'
---@type table<string, HullConfig.Slot>
HullConfig.slots = {
	sensor     = Slot:clone { type = "sensor",     size = 1 },
	computer_1 = Slot:clone { type = "computer",   size = 1 },
	computer_2 = Slot:clone { type = "computer",   size = 1 },
	hull_mod   = Slot:clone { type = "hull",       size = 1 },
	hyperdrive = Slot:clone { type = "hyperdrive", size = 1 },
	thruster   = Slot:clone { type = "thruster",   size = 1 },
	scoop      = Slot:clone { type = "scoop",      size = 1, hardpoint = true },
}

function HullConfig:__clone()
	self.slots = utils.map_table(self.slots, function(key, slot)
		return key, slot:clone()
	end)
end

Serializer:RegisterClass("HullConfig", HullConfig)
Serializer:RegisterClass("HullConfig.Slot", Slot)

--==============================================================================

local function CreateShipConfig(def)
	local newShip = HullConfig:clone()
	Serializer:RegisterPersistent("ShipDef." .. def.id, newShip)

	newShip.id = def.id
	newShip.capacity = def.capacity

	table.merge(newShip.slots, def.raw.equipment_slots or {}, function(name, slotDef)
		if slotDef == false then return name, nil end

		local newSlot = table.merge(Slot:clone(), slotDef)

		return name, newSlot
	end)

	for name, slot in pairs(newShip.slots) do
		slot.id = name
	end

	return newShip
end

---@type table<string, HullConfig>
local Configs = {}

for id, def in pairs(ShipDef) do
	if def.tag == "SHIP" or def.tag == "STATIC_SHIP" then
		Configs[id] = CreateShipConfig(def)
	end
end

local function GetHullConfigs()
	return Configs
end

--==============================================================================

return {
	Config = HullConfig,
	Slot = Slot,
	GetHullConfigs = GetHullConfigs
}
