// Copyright © 2008-2021 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#pragma once

#include "PowerSystem.h"
#include "Random.h"
#include "scenegraph/Model.h"

class Body;
class ShipType;
// simple thruster system
class SimplePowerSystem : public PowerSystem, public SimpleThrust, public Serializator {
	// body, we are connected to
	Body *m_body;
	const ShipType *m_params;
	Random rng;

	struct ThrusterUnit {
		vector3f pos;
		vector3f dir;
		Thruster direction;
	};

	// thrusters in the model
	std::vector<ThrusterUnit> m_thrusterUnits;

	// static parameters, determined by ship type
	float m_effectiveExhaustVelocity;

	// actual dynamic parameters
	vector3d m_force;
	vector3d m_torque;
	ThrustBox m_levels; // to consume fuel and render, maybe
	double m_fuel; // 0.0-1.0, remaining fuel

public:

	// we need the body that we will push, initial parameters, and the
	// model in to scan its thrusters
	SimplePowerSystem(Body *body, const ShipType *params, SceneGraph::Model *model);

	// fuel 0.0 .. 1.0
	double GetFuel() override final { return m_fuel; }
	void SetFuel(double f) override final { m_fuel = f; }
	double GetFuelUseRate(Thruster) override final;
	virtual float GetDeltaV(Thruster direction, float reserve) override final;

	// thrust
	double GetThrust(Thruster) override final;
	double GetAngThrust(int axis) override final;
	void SetThrustLevel(int axis, float level) override final;
	void SetAngThrustLevel(int axis, float level) override final;
	// for thruster model
	virtual float GetLevel(unsigned thrusterID) override final;
	
	// how this system acts on the body
	vector3d GetForce() override final { return m_force; }
	vector3d GetTorque() override final { return m_torque; }

	// timestep
	void UpdateFuel(float timeStep) override final;
	void ClearLinThrusterState() override final { m_force = vector3d(0.f); m_levels.fill(0.f); }
	void ClearAngThrusterState() override final { m_torque = vector3d(0.f); }

	vector3d &GetThrustLevels() override final { return m_force; } // XXX

	// serialization
	virtual void SaveToJson(Json &jsonObj) override final;
	virtual void LoadFromJson(Json &jsonObj) override final;

	SimpleThrust *GetSimpleThrust() & override final { return this; }
	Serializator *GetSerializator() & override final { return this; }
};
