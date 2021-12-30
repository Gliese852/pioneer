// Copyright © 2008-2021 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#pragma once

#include "PowerSystem.h"
#include "Random.h"
#include "scenegraph/Model.h"

// using model thrusters without body
class DemoPowerSystem : public PowerSystem {

	std::map<unsigned, ThrusterConfig::Type> fwdThrusters;
	Random rng;

public:
	DemoPowerSystem(SceneGraph::Model*);
	// fuel 0.0 .. 1.0
	double GetFuel() override final { return 0; }
	void SetFuel(double) override final {}
	double GetFuelUseRate(Thruster) override final { return 0; }
	float GetDeltaV(Thruster direction, float reserve) override final { return 0; }

	// thrust
	double GetThrust(Thruster) override final { return 0; }
	double GetAngThrust(int axis) override final { return 0; }
	vector3d GetForce() override final { return vector3d(0.f); }
	vector3d GetTorque() override final { return vector3d(0.f); }
	void SetThrustLevel(int axis, float level) override final {}
	void SetAngThrustLevel(int axis, float level) override final {}
	float GetLevel(unsigned thrusterID) override final;
	void UpdateFuel(float timeStep) override final {}
	void ClearLinThrusterState() override final {}
	void ClearAngThrusterState() override final {}
}; 
