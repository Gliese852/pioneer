// Copyright © 2008-2021 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#pragma once

#include "ship/ThrusterConfig.h"
#include "Json.h"

// extensions

struct SimpleThrust {
	virtual ~SimpleThrust(){}
	virtual vector3d &GetThrustLevels() = 0;
};

struct MultiMode {
	virtual ~MultiMode(){}
	virtual void SetThrusterMode(ThrusterConfig::Mode mode) = 0;
	virtual ThrusterConfig::Mode GetThrusterMode() = 0;
};

struct Upgrades {
	virtual ~Upgrades(){}
	virtual void SetThrustPowerMult(float p) = 0;
	virtual void SetAccelerationCapMult(float p) = 0;
};

struct Serializator {
	virtual ~Serializator(){}
	virtual void SaveToJson(Json &jsonObj) = 0;
	virtual void LoadFromJson(Json &jsonObj) = 0;
};

class PowerSystem {
public:

	virtual ~PowerSystem(){}

	// fuel 0.0 .. 1.0
	virtual double GetFuel() = 0;
	virtual void SetFuel(double) = 0;
	virtual double GetFuelUseRate(Thruster) = 0;
	virtual float GetDeltaV(Thruster direction, float reserve) = 0;
	virtual void UpdateFuel(float timeStep) = 0;

	// thrust
	virtual double GetThrust(Thruster) = 0;
	virtual double GetAngThrust(int axis) = 0;
	virtual void SetThrustLevel(int axis, float level) = 0;
	virtual void SetAngThrustLevel(int axis, float level) = 0;
	virtual void ClearLinThrusterState() = 0;
	virtual void ClearAngThrusterState() = 0;

	// forces
	virtual vector3d GetForce() = 0;
	virtual vector3d GetTorque() = 0;

	// for rendering a specific thruster
	virtual float GetLevel(unsigned thrusterID) = 0;

	// extension getters
	virtual MultiMode *GetMultiMode() & { return nullptr; }
	virtual Upgrades *GetUpgrades() & { return nullptr; }
	virtual SimpleThrust *GetSimpleThrust() & { return nullptr; }
	virtual Serializator *GetSerializator() & { return nullptr; }

}; 
