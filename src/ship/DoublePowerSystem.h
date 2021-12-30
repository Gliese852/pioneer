// Copyright © 2008-2021 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#pragma once

#include "PowerSystem.h"
#include "Random.h"
#include "scenegraph/Model.h"

class Body;
class ShipType;


// simple Main / Rcs thruster system
class DoublePowerSystem : public PowerSystem, public SimpleThrust, public Serializator, public MultiMode, public Upgrades {
	// body, we are connected to
	Body *m_body;
	const ShipType *m_params;
	Random rng;

	struct ThrusterUnit {
		vector3f pos;
		vector3f dir;
		Thruster direction;
		ThrusterConfig::Type type;
		bool linear;
	};

	// thrusters in the model
	std::vector<ThrusterUnit> m_thrusterUnits;

	// static parameters, determined by ship type
	std::array<float, ThrusterConfig::TYPE_MAX> m_effectiveExhaustVelocity;

	ThrusterConfig::Mode m_currentThrusterMode;

	// can be improved by upgrades
	ThrustBox m_linAccelerationCap;
	ThrusterArray m_thrusterPowers;
	double m_maxAngThrust;

	// actual dynamic parameters
	vector3d m_linThrust;
	vector3d m_angThrust;
	double m_fuel; // 0.0-1.0, remaining fuel

	// utility functions
	ThrusterConfig::Type GetActiveThrusterType(Thruster direction);

	double GetThrustUncapped(Thruster direction);

public:

	// we need the body that we will push, initial parameters, and the
	// model in to scan its thrusters
	DoublePowerSystem(Body *body, const ShipType *params, SceneGraph::Model *model);

	// fuel 0.0 .. 1.0
	double GetFuel() override final { return m_fuel; }
	void SetFuel(double f) override final;
	double GetFuelUseRate(Thruster) override final;
	virtual float GetDeltaV(Thruster direction, float reserve) override final;

	// thrust
	double GetThrust(Thruster) override final;
	double GetAngThrust(int axis) override final { return m_maxAngThrust; }
	void SetThrustLevel(int axis, float level) override final;
	void SetAngThrustLevel(int axis, float level) override final;
	// for thruster model
	virtual float GetLevel(unsigned thrusterID) override final;
	
	// how this system acts on the body
	vector3d GetForce() override final;
	vector3d GetTorque() override final;

	// timestep
	void UpdateFuel(float timeStep) override final;
	void ClearLinThrusterState() override final;
	void ClearAngThrusterState() override final;

	// updrades
	void SetThrustPowerMult(float p) override final;
	void SetAccelerationCapMult(float p) override final;

	// modes
	void SetThrusterMode(ThrusterConfig::Mode mode) override final { m_currentThrusterMode = mode; }
	ThrusterConfig::Mode GetThrusterMode()  override final { return m_currentThrusterMode; }

	// simple thrust
	vector3d &GetThrustLevels() override final { return m_linThrust; }

	// serialization
	virtual void SaveToJson(Json &jsonObj) override final;
	virtual void LoadFromJson(Json &jsonObj) override final;

	// extensions
	MultiMode *GetMultiMode() & override final { return this; }
	Upgrades *GetUpgrades() & override final { return this; }
	SimpleThrust *GetSimpleThrust() & override final { return this; }
	Serializator *GetSerializator() & override final { return this; }
};

