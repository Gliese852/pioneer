// Copyright © 2008-2021 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "SimplePowerSystem.h"
#include "ShipType.h"
#include "scenegraph/FindNodeVisitor.h"
#include "scenegraph/MatrixTransform.h"
#include "scenegraph/Thruster.h"

SimplePowerSystem::SimplePowerSystem(Body *body, const ShipType *params, SceneGraph::Model *model):
	PowerSystem(),
	m_body(body), m_params(params),
	m_effectiveExhaustVelocity(params->effectiveExhaustVelocity), //XXX
	m_force(0.f), m_torque(0.f),
	m_levels{0.f, 0.f, 0.f, 0.f, 0.f, 0.f},
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
	}
}

void SimplePowerSystem::SaveToJson(Json &jsonObj)
{
	jsonObj["thruster_fuel"] = m_fuel;
}

void SimplePowerSystem::LoadFromJson(Json &jsonObj)
{
	m_fuel = jsonObj["thruster_fuel"];
}

double SimplePowerSystem::GetFuelUseRate(Thruster direction)
{
	assert (m_params->fuelTankMass > 0);
	// fuelTankMass in tons, convert to kg
	return GetThrust(direction) / m_params->fuelTankMass / 1000.f / m_effectiveExhaustVelocity; // sec^-1
}

float SimplePowerSystem::GetDeltaV(Thruster direction, float reserve)
{
	if (m_fuel <= reserve) return 0.f;
	float mass = m_body->GetMass();
	float fuelmass = m_params->fuelTankMass * (m_fuel - reserve) * 1000.f;
	return m_effectiveExhaustVelocity * std::log(mass / (mass - fuelmass));
}

double SimplePowerSystem::GetThrust(Thruster direction)
{
	// we consider RCS as basic thrusters
	return m_params->linThrust[ThrusterConfig::TYPE_RCS][direction];
}

double SimplePowerSystem::GetAngThrust(int axis)
{
	return m_params->angThrust;
}

void SimplePowerSystem::SetThrustLevel(int axis, float level)
{
	assert(axis >= 0 && axis <= 2);
	if (m_fuel <= 0) return;
	Thruster direction = ThrusterConfig::ThrusterFromAxis(axis, level > 0);
	level = Clamp(level, -1.f, 1.f);
	float force = level * GetThrust(direction);
	m_force[axis] = force;
	m_levels[direction] = fabs(level);
	m_levels[ThrusterConfig::OtherSideThruster(direction)] = 0.f;
}

void SimplePowerSystem::SetAngThrustLevel(int axis, float level)
{
	assert(axis >= 0 && axis <= 2);
	if (m_fuel <= 0) return;
	m_torque[axis] = Clamp(level, -1.f, 1.f) * m_params->angThrust;
}


float SimplePowerSystem::GetLevel(unsigned id)
{
	ThrusterUnit &tu = m_thrusterUnits[id];
	// check thruster type 
		//float power = -dir.Dot(vector3f(rd->linthrust));
	float power = m_levels[tu.direction];

	// ang thrust
	const vector3f at (m_torque / -m_params->angThrust);
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

	//add some random
	return power * rng.Double_closed(0.8, 1.0); 
}


void SimplePowerSystem::UpdateFuel(float timeStep) {
	// update fuel
	for(unsigned i = 0; i < THRUSTER_MAX; ++i) {
		assert(m_levels[i] >= 0 && "Thruster level can't be < 0");
		if(m_levels[i] > 0) {
			m_fuel -= m_levels[i] * GetFuelUseRate(static_cast<Thruster>(i)) * timeStep;
		}
	}
}
