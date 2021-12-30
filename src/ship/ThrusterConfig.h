#ifndef THRUSTERCONFIG_H
#define THRUSTERCONFIG_H

#include "vector3.h"
#include <cassert>
#include <array>

enum Thruster { // <enum scope='Thruster' name=ShipTypeThruster prefix=THRUSTER_ public>
	THRUSTER_REVERSE = 0, // +Z
	THRUSTER_FORWARD = 1, // -Z
	THRUSTER_UP = 2,      // +Y
	THRUSTER_DOWN = 3,    // -Y
	THRUSTER_LEFT = 4,    // -X
	THRUSTER_RIGHT = 5,   // +X
	THRUSTER_MAX = 6 // <enum skip>
};

struct ThrusterConfig {

	// type of one specific thruster
	enum Type {
		TYPE_RCS,
		TYPE_MAIN,
		TYPE_MAX
	};

	// engine mode (for example, in the MODE_MAIN mode, RCS engines can also work
	// if there is no MAIN in this direction
	enum Mode {
		MODE_RCS,
		MODE_MAIN,
		MODE_MAX
	};

	Type type;
	// the engine does not participate in rotary motion
	bool isLinear;

	// given the thrust vector of the thruster in the ship's coordinate system.
	// the thruster is directed in the opposite direction to the thrust it creates,
	// e.g. TRUSTER_UP pointing down.
	static Thruster ThrusterFromDirection(vector3f dir)
	{
		assert(dir.Length() > 0.001 && "Thruster direction vector is too short");
		assert(dir.Length() < 1e38 && "Thruster direction vector is too long");
		dir = dir.Normalized();
		const double crit = 0.577; // sqrt(0.333...) side of a cube with a diagonal of 1
		if (dir.z < -crit) return THRUSTER_REVERSE;
		else if (dir.z > crit) return THRUSTER_FORWARD;
		else if (dir.y < -crit) return THRUSTER_UP;
		else if (dir.y > crit) return THRUSTER_DOWN;
		else if (dir.x > crit) return THRUSTER_LEFT;
		else if (dir.x < -crit) return THRUSTER_RIGHT;
		else assert(false && "Impossible thruster direction vector");
		return THRUSTER_REVERSE;
	}

	template<class T>
	static T ThrustFromVector(vector3<T> vec, Thruster direction)
	{
		switch (direction) {
			case THRUSTER_FORWARD: return vec.z < 0 ? -vec.z : 0;
			case THRUSTER_REVERSE: return vec.z > 0 ? vec.z : 0;
			case THRUSTER_UP:      return vec.y > 0 ? vec.y : 0;
			case THRUSTER_DOWN:    return vec.y < 0 ? -vec.y : 0;
			case THRUSTER_LEFT:    return vec.x < 0 ? -vec.x : 0;
			case THRUSTER_RIGHT:   return vec.x > 0 ? vec.x : 0;
			default: assert(0 && "Invalid thrust direction");
		}
		return 0;
	}

	static Thruster OtherSideThruster(Thruster direction)
	{
		switch (direction) {
			case THRUSTER_FORWARD: return THRUSTER_REVERSE;
			case THRUSTER_REVERSE: return THRUSTER_FORWARD;
			case THRUSTER_UP: return THRUSTER_DOWN;
			case THRUSTER_DOWN: return THRUSTER_UP;
			case THRUSTER_LEFT: return THRUSTER_RIGHT;
			case THRUSTER_RIGHT: return THRUSTER_LEFT;
			default: assert(0 && "Invalid thrust direction");
		}
		// never be here
		return THRUSTER_MAX;
	};

	static Thruster ThrusterFromAxis(int axis, bool positive) {
		switch (axis) {
			case 0: return positive ? THRUSTER_RIGHT : THRUSTER_LEFT;
			case 1: return positive ? THRUSTER_UP : THRUSTER_DOWN;
			case 2: return positive ? THRUSTER_REVERSE : THRUSTER_FORWARD;
			default: assert(0 && "Axis must be 0..2"); return THRUSTER_REVERSE;
		}
	}
};

using ThrustBox = std::array<float, THRUSTER_MAX>;
using ThrusterArray = std::array<ThrustBox, ThrusterConfig::MODE_MAX>;
using ThrusterModes = std::array<std::array<ThrusterConfig::Type, THRUSTER_MAX>, ThrusterConfig::MODE_MAX>;

#endif // THRUSTERCONFIG_H
