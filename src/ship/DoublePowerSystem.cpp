// Copyright © 2008-2021 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "DoublePowerSystem.h"
#include "ShipType.h"
#include "scenegraph/FindNodeVisitor.h"
#include "scenegraph/MatrixTransform.h"
#include "scenegraph/Thruster.h"
#include "Sfx.h"
#include "ship/PowerSystem.h"

DoublePowerSystem::DoublePowerSystem(Body *body, const ShipType *params, SceneGraph::Model *model):
	PowerSystem(),
	m_body(body), m_params(params),
	// hardcode than RCS is 2 times less efficient than MAIN
	m_effectiveExhaustVelocity{params->effectiveExhaustVelocity, params->effectiveExhaustVelocity},
	m_currentThrusterMode(ThrusterConfig::MODE_RCS),
	m_linAccelerationCap(params->linAccelerationCap),
	m_thrusterPowers(params->linThrust),
	m_maxAngThrust(params->angThrust),
	m_linThrust(0.f), m_angThrust(0.f),
	m_fuel(0.0)
{
	// * scan the model looking for thrusters
	// get thrusters branch
	SceneGraph::FindNodeVisitor thrusterFinder(SceneGraph::FindNodeVisitor::MATCH_NAME_FULL, "thrusters");
	model->GetRoot()->Accept(thrusterFinder);
	auto &results = thrusterFinder.GetResults();
	auto *thrusters = static_cast<SceneGraph::Group *>(results.at(0));
	// fill in the vector, by indices corresponding to the thruster's index
	m_thrusterUnits.reserve(thrusters->GetNumChildren());
	m_thrusterUnits.resize(thrusters->GetNumChildren());
	for (unsigned i = 0; i < thrusters->GetNumChildren(); i++) {
		auto *mt = static_cast<SceneGraph::MatrixTransform *>(thrusters->GetChildAt(i));
		auto *my_thruster = static_cast<SceneGraph::Thruster *>(mt->GetChildAt(0));
		assert(my_thruster->GetID() < thrusters->GetNumChildren());
		ThrusterUnit &tu = m_thrusterUnits[my_thruster->GetID()];
		tu.pos = mt->GetTransform().GetTranslate();
		tu.dir = mt->GetTransform().GetOrient() * vector3f(0.f, 0.f, 1.f);
		tu.direction = ThrusterConfig::ThrusterFromDirection(tu.dir);
		tu.type = my_thruster->GetConfig().type;
		tu.linear = my_thruster->GetConfig().isLinear;
	}
}

void DoublePowerSystem::ClearLinThrusterState()
{
	// if(m_body->IsType(ObjectType::PLAYER)) Output("PLAYER_CLEAR_LIN_THRUSTER_STATE!\n");
	m_linThrust = vector3d(0.0);
}

vector3d DoublePowerSystem::GetForce()
{
	assert(m_linThrust.x >= -1.f && m_linThrust.x <= 1.f);
	assert(m_linThrust.y >= -1.f && m_linThrust.y <= 1.f);
	assert(m_linThrust.z >= -1.f && m_linThrust.z <= 1.f);
	return {
		m_linThrust.x * GetThrustUncapped(ThrusterConfig::ThrusterFromAxis(0, m_linThrust.x > 0.f)),
		m_linThrust.y * GetThrustUncapped(ThrusterConfig::ThrusterFromAxis(1, m_linThrust.y > 0.f)),
		m_linThrust.z * GetThrustUncapped(ThrusterConfig::ThrusterFromAxis(2, m_linThrust.z > 0.f))
	};
}

vector3d DoublePowerSystem::GetTorque() {
	return m_angThrust * m_maxAngThrust;
}

void DoublePowerSystem::ClearAngThrusterState()
{
	m_angThrust = vector3d(0.0);
}

void DoublePowerSystem::SaveToJson(Json &jsonObj)
{
	jsonObj["thruster_fuel"] = m_fuel;
	jsonObj["thruster_mode"] = m_currentThrusterMode;
}

void DoublePowerSystem::LoadFromJson(Json &jsonObj)
{
	m_fuel = jsonObj["thruster_fuel"];
	m_currentThrusterMode = jsonObj["thruster_mode"];
}

double DoublePowerSystem::GetFuelUseRate(Thruster direction)
{
	assert (m_params->fuelTankMass > 0);
	auto type = static_cast<int>(GetActiveThrusterType(direction));
	// fuelTankMass in tons, convert to kg
	return GetThrustUncapped(direction) / m_params->fuelTankMass / 1000.f / m_effectiveExhaustVelocity[type]; // sec^-1
}

float DoublePowerSystem::GetDeltaV(Thruster direction, float reserve)
{
	if (m_fuel <= reserve) return 0.f;
	auto type = static_cast<int>(GetActiveThrusterType(direction)); // main or RCS
	float mass = m_body->GetMass();
	float fuelmass = m_params->fuelTankMass * (m_fuel - reserve) * 1000.f;
	return m_effectiveExhaustVelocity[type] * std::log(mass / (mass - fuelmass));
}

double DoublePowerSystem::GetThrustUncapped(Thruster direction)
{
	return m_thrusterPowers[m_currentThrusterMode][direction];
}

double DoublePowerSystem::GetThrust(Thruster direction)
{
	assert(m_body->GetMass() > 0);
	double capped = m_linAccelerationCap[direction] * m_body->GetMass();
	return std::min(capped, GetThrustUncapped(direction));
}

// a thrust of 1.0 must correspond to a maximum thrust not exceeding the acceleration limit in a given direction
void DoublePowerSystem::SetThrustLevel(int axis, float level)
{
	assert(axis >= 0 && axis <= 2);
	if (m_fuel <= 0) return;
	Thruster direction = ThrusterConfig::ThrusterFromAxis(axis, level > 0);
	float ratio = GetThrust(direction) / GetThrustUncapped(direction);
	// don't want to check the sign again, just clamp both sides
	m_linThrust[axis] = Clamp(level, -ratio, ratio);
}

void DoublePowerSystem::SetAngThrustLevel(int axis, float level)
{
	assert(axis >= 0 && axis <= 2);
	if (m_fuel <= 0) return;
	m_angThrust[axis] = Clamp(level, -1.f, 1.f);
}

float DoublePowerSystem::GetLevel(unsigned id)
{
	ThrusterUnit &tu = m_thrusterUnits[id];
	// check thruster type 
	float power;
	if(GetActiveThrusterType(tu.direction) == tu.type) {
		power = ThrusterConfig::ThrustFromVector(m_linThrust, tu.direction);
	} else {
		if(tu.linear) {
			return 0.f;
		} else {
			power = 0.f;
		}
	}

	// angthrust
	if(!tu.linear) {
		// pitch X
		// yaw   Y
		// roll  Z

		const vector3f at(m_angThrust);
		const vector3f angdir = tu.pos.Cross(tu.dir);

		const float xp = angdir.x * at.x;
		const float yp = angdir.y * at.y;
		const float zp = angdir.z * at.z;

		if (xp + yp + zp > 0.001) {
			if (xp > yp && xp > zp && fabs(at.x) > power)
				power = fabs(at.x);
			else if (yp > xp && yp > zp && fabs(at.y) > power)
				power = fabs(at.y);
			else if (zp > xp && zp > yp && fabs(at.z) > power)
				power = fabs(at.z);
		}
	}

	//add some random
	return power * rng.Double_closed(0.8, 1.0); 
}

void DoublePowerSystem::SetFuel(double f) {
	m_fuel = f; 
}


void DoublePowerSystem::UpdateFuel(float timeStep) {
	if (m_fuel <= 0) return;
	for(unsigned i = 0; i < 3; ++i) {
		Thruster direction = ThrusterConfig::ThrusterFromAxis(i, m_linThrust[i] > 0);
		double level = fabs(m_linThrust[i]);
		assert(level >= 0 && level <= 1.0f);
		if(level > 0) {
			m_fuel -= level * GetFuelUseRate(direction) * timeStep;
		}
	}
}

ThrusterConfig::Type DoublePowerSystem::GetActiveThrusterType(Thruster direction)
{
	return m_params->thrusterModes[m_currentThrusterMode][direction];
}

void DoublePowerSystem::SetThrustPowerMult(float p)
{
	for (int i = 0; i < ThrusterConfig::MODE_MAX; i++) {
		for (int j = 0; j < THRUSTER_MAX; ++j) {
			m_thrusterPowers[i][j] = m_params->linThrust[i][j] * p;
		}
	}
	m_maxAngThrust = m_params->angThrust * p;
}

void DoublePowerSystem::SetAccelerationCapMult(float p)
{
	for (int i = 0; i < Thruster::THRUSTER_MAX; i++) {
		m_linAccelerationCap[i] = m_params->linAccelerationCap[i] * p;
	}
}
