// Copyright © 2008-2021 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "Propulsion.h"

#include "Body.h"
#include "Game.h"
#include "GameSaveError.h"
#include "Pi.h"
#include "Player.h"
#include "PlayerShipController.h"
#include "PowerSystem.h"
#include "enum_table.h"
#include "EnumStrings.h"
#include "fmt/format.h"
#include "ship/ThrusterConfig.h"



void Propulsion::SaveToJson(Json &jsonObj, Space *space)
{
	//Json PropulsionObj(Json::objectValue); // Create JSON object to contain propulsion data.
	//jsonObj["ang_thrusters"] = m_angThrusters;
	// jsonObj["thrusters"] = m_linThrusters;
	// jsonObj["thruster_fuel"] = m_thrusterFuel;
	jsonObj["reserve_fuel"] = m_reserveFuel;
	// !!! These are commented to avoid savegame bumps:
	//jsonObj["tank_mass"] = m_fuelTankMass;
	//jsonObj["propulsion"] = PropulsionObj;
}

void Propulsion::LoadFromJson(const Json &jsonObj, Space *space)
{
	try {
		//SetAngThrusterState(jsonObj["ang_thrusters"]);
		//SetLinThrusterState(jsonObj["thrusters"]);

		//m_thrusterFuel = jsonObj["thruster_fuel"];
		m_reserveFuel = jsonObj["reserve_fuel"];

		// !!! This is commented to avoid savegame bumps:
		//m_fuelTankMass = jsonObj["tank_mass"].asInt();
	} catch (Json::type_error &) {
		throw SavedGameCorruptException();
	}
}

Propulsion::Propulsion()
{
	m_reserveFuel = 0.0;
	m_fuelStateChange = false;
	m_dBody = nullptr;
	m_engine = nullptr;
}

void Propulsion::Init(DynamicBody *b, PowerSystem *engine)
{
	m_dBody = b;
	m_engine = engine;
}


void Propulsion::SetAngThrusterState(const vector3d &levels)
{
	for(auto axis : { 0, 1, 2 }) {
		m_engine->SetAngThrustLevel(axis, levels[axis]);
	}
}

void Propulsion::SetLinThrusterState(int axis, double level)
{
	// if (m_dBody->IsType(ObjectType::PLAYER)){ Output("prop_slts_axis: %d level %f\n", axis, level); }
	m_engine->SetThrustLevel(axis, level);
}

void Propulsion::SetLinThrusterState(const vector3d &levels)
{
	if (m_dBody->IsType(ObjectType::PLAYER)){
		//Output("prop_slts_vec: "); levels.Print();
	}
	for(auto axis : { 0, 1, 2 }) {
		m_engine->SetThrustLevel(axis, levels[axis]);
	}
}

vector3d Propulsion::GetThrust(const vector3d &dir) const
{
	vector3d maxThrust;

	maxThrust.x = (dir.x > 0) ? m_engine->GetThrust(THRUSTER_RIGHT) : m_engine->GetThrust(THRUSTER_LEFT);
	maxThrust.y = (dir.y > 0) ? m_engine->GetThrust(THRUSTER_UP) : m_engine->GetThrust(THRUSTER_DOWN);
	maxThrust.z = (dir.z > 0) ? m_engine->GetThrust(THRUSTER_REVERSE) : m_engine->GetThrust(THRUSTER_FORWARD);

	return maxThrust;
}

double Propulsion::GetThrustMin() const
{
	// These are the weakest thrusters in a ship
	double val = static_cast<double>(m_engine->GetThrust(THRUSTER_UP));
	val = std::min(val, static_cast<double>(m_engine->GetThrust(THRUSTER_RIGHT)));
	val = std::min(val, static_cast<double>(m_engine->GetThrust(THRUSTER_LEFT)));
	return val;
}

void Propulsion::UpdateFuel(const float timeStep)
{
	FuelState lastState = GetFuelState();
	m_engine->UpdateFuel(timeStep);
	FuelState currentState = GetFuelState();
	if (currentState != lastState)
		m_fuelStateChange = true;
	else
		m_fuelStateChange = false;
}

// returns speed that can be reached using fuel minus reserve according to the Tsiolkovsky equation
double Propulsion::GetSpeedReachedWithFuel() const
{
	return m_engine->GetDeltaV(THRUSTER_FORWARD, m_reserveFuel);
}

void Propulsion::AIModelCoordsMatchAngVel(const vector3d &desiredAngVel, double softness)
{
	double angAccel = m_engine->GetAngThrust(0) / m_dBody->GetAngularInertia();
	const double softTimeStep = Pi::game->GetTimeStep() * softness;

	vector3d angVel = desiredAngVel - m_dBody->GetAngVelocity() * m_dBody->GetOrient();
	vector3d thrust;
	for (int axis = 0; axis < 3; axis++) {
		if (angAccel * softTimeStep >= fabs(angVel[axis])) {
			thrust[axis] = angVel[axis] / (softTimeStep * angAccel);
		} else {
			thrust[axis] = (angVel[axis] > 0.0 ? 1.0 : -1.0);
		}
	}
	SetAngThrusterState(thrust);
}

void Propulsion::AIModelCoordsMatchSpeedRelTo(const vector3d &v, const DynamicBody *other)
{
	vector3d relToVel = other->GetVelocity() * m_dBody->GetOrient() + v;
	AIAccelToModelRelativeVelocity(relToVel);
}

// Try to reach this model-relative velocity.
// (0,0,-100) would mean going 100m/s forward.

void Propulsion::AIAccelToModelRelativeVelocity(const vector3d &v)
{
	vector3d difVel = v - m_dBody->GetVelocity() * m_dBody->GetOrient(); // required change in velocity
	vector3d maxThrust = GetThrust(difVel);
	vector3d maxFrameAccel = maxThrust * (Pi::game->GetTimeStep() / m_dBody->GetMass());

	SetLinThrusterState(0, is_zero_exact(maxFrameAccel.x) ? 0.0 : difVel.x / maxFrameAccel.x);
	SetLinThrusterState(1, is_zero_exact(maxFrameAccel.y) ? 0.0 : difVel.y / maxFrameAccel.y);
	SetLinThrusterState(2, is_zero_exact(maxFrameAccel.z) ? 0.0 : difVel.z / maxFrameAccel.z); // use clamping
}

/* NOTE: following code were in Ship-AI.cpp file,
 * no changes were made, except those needed
 * to make it compatible with actual Propulsion
 * class (and yes: it's only a copy-paste,
 * including comments :) )
*/

// Because of issues when reducing timestep, must do parts of this as if 1x accel
// final frame has too high velocity to correct if timestep is reduced
// fix is too slow in the terminal stages:
//	if (endvel <= vel) { endvel = vel; ivel = dist / Pi::game->GetTimeStep(); }	// last frame discrete correction
//	ivel = std::min(ivel, endvel + 0.5*acc/PHYSICS_HZ);	// unknown next timestep discrete overshoot correction

// yeah ok, this doesn't work
// sometimes endvel is too low to catch moving objects
// worked around with half-accel hack in dynamicbody & pi.cpp

double calc_ivel(double dist, double vel, double acc)
{
	bool inv = false;
	if (dist < 0) {
		dist = -dist;
		vel = -vel;
		inv = true;
	}
	double ivel = 0.9 * sqrt(vel * vel + 2.0 * acc * dist); // fudge hardly necessary

	double endvel = ivel - (acc * Pi::game->GetTimeStep());
	if (endvel <= 0.0)
		ivel = dist / Pi::game->GetTimeStep(); // last frame discrete correction
	else
		ivel = (ivel + endvel) * 0.5; // discrete overshoot correction
	//	else ivel = endvel + 0.5*acc/PHYSICS_HZ;                  // unknown next timestep discrete overshoot correction

	return (inv) ? -ivel : ivel;
}

// version for all-positive values
double calc_ivel_pos(double dist, double vel, double acc)
{
	double ivel = 0.9 * sqrt(vel * vel + 2.0 * acc * dist); // fudge hardly necessary

	double endvel = ivel - (acc * Pi::game->GetTimeStep());
	if (endvel <= 0.0)
		ivel = dist / Pi::game->GetTimeStep(); // last frame discrete correction
	else
		ivel = (ivel + endvel) * 0.5; // discrete overshoot correction

	return ivel;
}

// vel is desired velocity in ship's frame
// returns true if this can be attained in a single timestep
bool Propulsion::AIMatchVel(const vector3d &vel)
{
	vector3d diffvel = (vel - m_dBody->GetVelocity()) * m_dBody->GetOrient();
	return AIChangeVelBy(diffvel);
}

// diffvel is required change in velocity in object space
// returns true if this can be done in a single timestep
bool Propulsion::AIChangeVelBy(const vector3d &diffvel)
{
	// counter external forces
	vector3d extf = m_dBody->GetExternalForce() * (Pi::game->GetTimeStep() / m_dBody->GetMass());
	vector3d diffvel2 = diffvel - extf * m_dBody->GetOrient();

	vector3d maxThrust = GetThrust(diffvel2);
	vector3d maxFrameAccel = maxThrust * (Pi::game->GetTimeStep() / m_dBody->GetMass());
	vector3d thrust(diffvel2.x / maxFrameAccel.x,
		diffvel2.y / maxFrameAccel.y,
		diffvel2.z / maxFrameAccel.z);
	SetLinThrusterState(thrust); // use clamping
	if (thrust.x * thrust.x > 1.0 || thrust.y * thrust.y > 1.0 || thrust.z * thrust.z > 1.0) return false;
	return true;
}

// Change object-space velocity in direction of param
vector3d Propulsion::AIChangeVelDir(const vector3d &reqdiffvel)
{
	//maximum thrust along the axes coinciding in sign with the required direction
	vector3d maxthrust = GetThrust(reqdiffvel);
	vector3d corrthrust = maxthrust;
	// next vector is a special "thrust" space, where the the axes are
	// flipped so that the thrust vector is positive regardless of the
	// direction of the required velocity
	vector3d flip (1.0, 1.0, 1.0);
	const double eps = 1e-5; // precision
	if(reqdiffvel.x < 0) flip.x *= -1;
	if(reqdiffvel.y < 0) flip.y *= -1;
	if(reqdiffvel.z < 0) flip.z *= -1;
	vector3d extf = m_dBody->GetExternalForce() * m_dBody->GetOrient();
	corrthrust += extf * flip;
	corrthrust.x = std::max(corrthrust.x, 0.0);
	corrthrust.y = std::max(corrthrust.y, 0.0);
	corrthrust.z = std::max(corrthrust.z, 0.0);
	// the actual thrust vector must be proportional to the velocity vector
	// in "thrust" space
	vector3d thrust = reqdiffvel * flip;

	// first scale iteration
	if(thrust.x > eps) thrust *= corrthrust.x / thrust.x;
	else if (thrust.y > eps) thrust *= corrthrust.y / thrust.y;
	else if (thrust.z > eps) thrust *= corrthrust.z / thrust.z;
	else {
		// thrust + external forces is zero
		// do maximum thrust
		// need to pass levels -1.0..1.0 in model space
		SetLinThrusterState(flip);
		return vector3d(0.0);
	}

	// scale further
	if(thrust.x > eps && corrthrust.x < thrust.x) thrust *= corrthrust.x / thrust.x;
	if(thrust.y > eps && corrthrust.y < thrust.y) thrust *= corrthrust.y / thrust.y;
	if(thrust.y > eps && corrthrust.z < thrust.z) thrust *= corrthrust.z / thrust.z;

	// back into normal space, get back external forces

	thrust = thrust * flip - extf;
	vector3d levels (thrust.x / maxthrust.x, thrust.y / maxthrust.y, thrust.z / maxthrust.z);

	SetLinThrusterState(levels);
	return vector3d(0.0);
}

// Input in object space
void Propulsion::AIMatchAngVelObjSpace(const vector3d &angvel)
{
	double maxAccel = m_engine->GetAngThrust(0) / m_dBody->GetAngularInertia();
	double invFrameAccel = 1.0 / (maxAccel * Pi::game->GetTimeStep());

	vector3d diff = angvel - m_dBody->GetAngVelocity() * m_dBody->GetOrient(); // find diff between current & desired angvel
	SetAngThrusterState(diff * invFrameAccel);
}

// get updir as close as possible just using roll thrusters
double Propulsion::AIFaceUpdir(const vector3d &updir, double av)
{
	double maxAccel = m_engine->GetAngThrust(0) / m_dBody->GetAngularInertia(); // should probably be in stats anyway
	double frameAccel = maxAccel * Pi::game->GetTimeStep();

	vector3d uphead = updir * m_dBody->GetOrient(); // create desired object-space updir
	// it seems you can pass a vector of any length to this function
	// make sure we can normalize
	if(uphead.LengthSqr() < 1e-10) return 0;
	uphead = uphead.Normalized();
	// cosine of the angle sharper than which we think we are approaching gimbal lock
	if(m_dBody->IsType(ObjectType::PLAYER)) {
		Output("UpDir: uphead: %7.4f %7.4f %7.4f", uphead.x, uphead.y, uphead.z);
	}
	constexpr double LIMIT_COS = 0.93969;
	// bail out if facing almost down of almost up
	if (uphead.z > LIMIT_COS || uphead.z < -LIMIT_COS) { 
		if(m_dBody->IsType(ObjectType::PLAYER)) {
			Output("|bail out\n");
		}
		return 0; }
	uphead.z = 0;
	uphead = uphead.Normalized(); // only care about roll axis

	if(m_dBody->IsType(ObjectType::PLAYER)) {
		Output("|z0: %9.6f %9.6f %9.6f", uphead.x, uphead.y, uphead.z);
	}

	double ang = acos(uphead.y);					 // scalar angle from head to curhead
	double iangvel = 0.0 + calc_ivel_pos(ang, 0.0, maxAccel); // ideal angvel at current time
	double dav = uphead.x > 0 ? -iangvel : iangvel;
	double cav = (m_dBody->GetAngVelocity() * m_dBody->GetOrient()).z; // current obj-rel angvel
	double diff = (dav - cav) / frameAccel;							   // find diff between current & desired angvel

	SetAngThrusterState(2, diff);
	if(m_dBody->IsType(ObjectType::PLAYER)) {
		Output("|ang:%9.6f|dav:%7.4f|cav:%7.4f", ang, dav, cav);
		Output("|diff: %7.4f\n", diff);
	}
	return ang;
}

// get updir as close as possible just using roll thrusters
double Propulsion::AIFaceUpdirPitch(const vector3d &updir, double av)
{
	double maxAccel = m_engine->GetAngThrust(0) / m_dBody->GetAngularInertia(); // should probably be in stats anyway
	//double frameAccel = maxAccel * Pi::game->GetTimeStep();

	vector3d uphead = updir * m_dBody->GetOrient(); // create desired object-space updir
	if (uphead.z < -0.99999) return 0;			// bail out if facing up
	if (uphead.z > 0.99999) return 0;				// bail out if facing down
	if (uphead.y > 0.999999) {
		AIModelCoordsMatchAngVel(vector3d(0.0), 1.0);
		return 0;        // stop rotation and bail out if up is up
	}
	// rotation axis
	vector3d axis = vector3d(0.0, 1.0, 0.0).Cross(uphead).Normalized();
	auto ang = acos(uphead.Dot(vector3d(0.0, 1.0, 0.0)));
	auto want_rot = abs(ang);
	auto good_speed = sqrt(2 * maxAccel * want_rot) * 0.9;
	if(m_dBody->IsType(ObjectType::PLAYER)) {
		Output("faceup: ang: %.5f good_speed: %.5f axis(%.5f): ", ang, good_speed, axis.Length()); axis.Print();
	}
	AIModelCoordsMatchAngVel(axis * good_speed, 1.0);
	return ang;
}

// Input: direction in ship's frame, doesn't need to be normalized
// Approximate positive angular velocity at match point
// Applies thrust directly
// old: returns whether it can reach that direction in this frame
// returns angle to target
double Propulsion::AIFaceDirection(const vector3d &dir, double av)
{
	double maxAccel = m_engine->GetAngThrust(0) / m_dBody->GetAngularInertia(); // should probably be in stats anyway

	vector3d head = (dir * m_dBody->GetOrient()).Normalized(); // create desired object-space heading
	vector3d dav(0.0, 0.0, 0.0);							   // desired angular velocity

	double ang = 0.0;
	ang = acos(-head.z);					 // scalar angle from head to curhead
	double iangvel = av + calc_ivel_pos(ang, 0.0, maxAccel); // ideal angvel at current time

	// Normalize (head.x, head.y) to give desired angvel direction
	if (head.z > 0.999999) head.x = 1.0;
	double head2dnorm = 1.0 / sqrt(head.x * head.x + head.y * head.y); // NAN fix shouldn't be necessary if inputs are normalized
	dav.x = head.y * head2dnorm * iangvel;
	dav.y = -head.x * head2dnorm * iangvel;
	const vector3d cav = m_dBody->GetAngVelocity() * m_dBody->GetOrient(); // current obj-rel angvel
	const double frameAccel = maxAccel * Pi::game->GetTimeStep();
	vector3d diff = is_zero_exact(frameAccel) ? vector3d(0.0) : (dav - cav) / frameAccel; // find diff between current & desired angvel

	// If the player is pressing a roll key, don't override roll.
	// HACK this really shouldn't be here. a better way would be to have a
	// field in Ship describing the wanted angvel adjustment from input. the
	// baseclass version in Ship would always be 0. the version in Player
	// would be constructed from user input. that adjustment could then be
	// considered by this method when computing the required change
	if (m_dBody->IsType(ObjectType::PLAYER)) {
		auto *playerController = static_cast<const Player *>(m_dBody)->GetPlayerController();
		if (playerController->InputBindings.roll->IsActive())
			diff.z = GetAngThrusterState().z;
	}

	SetAngThrusterState(diff);
	return ang;
}

// returns direction in ship's frame from this ship to target lead position
vector3d Propulsion::AIGetLeadDir(const Body *target, const vector3d &targaccel, double projspeed)
{
	assert(target);
	const vector3d targpos = target->GetPositionRelTo(m_dBody);
	const vector3d targvel = target->GetVelocityRelTo(m_dBody);
	// todo: should adjust targpos for gunmount offset
	vector3d leadpos;
	// avoid a divide-by-zero floating point exception (very nearly zero is ok)
	if (!is_zero_exact(projspeed)) {
		// first attempt
		double projtime = targpos.Length() / projspeed;
		leadpos = targpos + targvel * projtime + 0.5 * targaccel * projtime * projtime;

		// second pass
		projtime = leadpos.Length() / projspeed;
		leadpos = targpos + targvel * projtime + 0.5 * targaccel * projtime * projtime;
	} else {
		// default
		leadpos = targpos;
	}
	return leadpos.Normalized();
}

vector3d Propulsion::GetLinThrusterState() const
{
	vector3d force(m_engine->GetForce());
	force.x /= m_engine->GetThrust(force.x > 0 ? THRUSTER_RIGHT : THRUSTER_LEFT);
	force.y /= m_engine->GetThrust(force.y > 0 ? THRUSTER_UP : THRUSTER_DOWN);
	force.z /= m_engine->GetThrust(force.z > 0 ? THRUSTER_REVERSE : THRUSTER_FORWARD);
	return force;
}

vector3d Propulsion::GetAngThrusterState() const
{
	vector3d ans(m_engine->GetTorque());
	ans.x /= ans.x * m_engine->GetAngThrust(0);
	ans.y /= ans.y * m_engine->GetAngThrust(1);
	ans.z /= ans.z * m_engine->GetAngThrust(2);
	return ans;
}
