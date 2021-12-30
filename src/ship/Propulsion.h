// Copyright © 2008-2021 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef PROPULSION_H
#define PROPULSION_H

#include "DynamicBody.h"
#include "JsonUtils.h"
#include "scenegraph/Model.h"
#include "vector3.h"
#include "ThrusterConfig.h"
#include "PowerSystem.h"

class Camera;
class Space;
struct ShipType;


class Propulsion : public RefCounted {
public:
	// Inits:
	Propulsion();
	virtual ~Propulsion(){};
	// Acceleration cap is infinite
	void Init(DynamicBody *b, PowerSystem *engine);

	virtual void SaveToJson(Json &jsonObj, Space *space);
	virtual void LoadFromJson(const Json &jsonObj, Space *space);

	// Thrust and thruster functions
	// Everything's capped unless specified otherwise.
	vector3d GetThrust(const vector3d &dir) const;
	inline double GetThrustFwd() const { return m_engine->GetThrust(THRUSTER_FORWARD); }
	inline double GetThrustRev() const { return m_engine->GetThrust(THRUSTER_REVERSE); }
	inline double GetThrustUp() const { return m_engine->GetThrust(THRUSTER_UP); }
	double GetThrustMin() const;

	inline double GetAccel(Thruster thruster) const { return m_engine->GetThrust(thruster) / m_dBody->GetMass(); }
	inline double GetAccelFwd() const { return GetAccel(THRUSTER_FORWARD); } //GetThrustFwd() / m_dBody->GetMass(); }
	inline double GetAccelRev() const { return GetAccel(THRUSTER_REVERSE); } //GetThrustRev() / m_dBody->GetMass(); }
	inline double GetAccelUp() const { return GetAccel(THRUSTER_UP); } //GetThrustUp() / m_dBody->GetMass(); }
	inline double GetAccelMin() const { return GetThrustMin() / m_dBody->GetMass(); }

	// A level of 1 corresponds to the thrust from GetThrust().
	void SetLinThrusterState(int axis, double level);
	void SetLinThrusterState(const vector3d &levels);

	inline void SetAngThrusterState(int axis, double level) { m_engine->SetAngThrustLevel(axis, level); }
	void SetAngThrusterState(const vector3d &levels);

	vector3d GetLinThrusterState() const;
	vector3d GetAngThrusterState() const;

	PowerSystem &GetEngine() { return *m_engine; }

	// Fuel
	enum FuelState { // <enum scope='Propulsion' name=PropulsionFuelStatus prefix=FUEL_ public>
		FUEL_OK,
		FUEL_WARNING,
		FUEL_EMPTY,
	};

	inline FuelState GetFuelState() const { return (m_engine->GetFuel() > 0.05f) ? FUEL_OK : (m_engine->GetFuel() > 0.0f) ? FUEL_WARNING : FUEL_EMPTY; }
	// fuel left, 0.0-1.0
	inline double GetFuel() const { return m_engine->GetFuel(); } //XXX?
	inline double GetFuelReserve() const { return m_reserveFuel; }
	inline void SetFuel(const double f) { m_engine->SetFuel(f); }
	inline void SetFuelReserve(const double f) { m_reserveFuel = Clamp(f, 0.0, 1.0); }
	// available delta-V given the ship's current fuel minus reserve according to the Tsiolkovsky equation
	double GetSpeedReachedWithFuel() const;
	/* TODO: These are needed to avoid savegamebumps:
		 * are used to pass things to/from shipStats;
		 * may be better if you not expose these fields
		*/
	void UpdateFuel(const float timeStep);
	inline bool IsFuelStateChanged() { return m_fuelStateChange; }

	// AI on Propulsion
	void AIModelCoordsMatchAngVel(const vector3d &desiredAngVel, double softness);
	void AIModelCoordsMatchSpeedRelTo(const vector3d &v, const DynamicBody *other);
	void AIAccelToModelRelativeVelocity(const vector3d &v);

	bool AIMatchVel(const vector3d &vel);
	bool AIChangeVelBy(const vector3d &diffvel); // acts in object space
	vector3d AIChangeVelDir(const vector3d &diffvel); // object space, maintain direction
	void AIMatchAngVelObjSpace(const vector3d &angvel);
	double AIFaceUpdir(const vector3d &updir, double av = 0);
	double AIFaceUpdirPitch(const vector3d &updir, double av = 0);
	double AIFaceDirection(const vector3d &dir, double av = 0);
	vector3d AIGetLeadDir(const Body *target, const vector3d &targaccel, double projspeed);

private:
	// Thrust and thrusters
	PowerSystem *m_engine;

	// Fuel
	double m_reserveFuel; // 0.0-1.0, fuel not to touch for the current AI program
	bool m_fuelStateChange;
	uint8_t m_thrusterConfig;

	const DynamicBody *m_dBody;
};

#endif // PROPULSION_H
