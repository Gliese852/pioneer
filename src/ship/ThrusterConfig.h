#ifndef THRUSTERCONFIG_H
#define THRUSTERCONFIG_H

#include "vector3.h"
#include <cassert>

enum Thruster { // <enum scope='Thruster' name=ShipTypeThruster prefix=THRUSTER_ public>
	THRUSTER_REVERSE = 0,
	THRUSTER_FORWARD = 1,
	THRUSTER_UP = 2,
	THRUSTER_DOWN = 3,
	THRUSTER_LEFT = 4,
	THRUSTER_RIGHT = 5,
	THRUSTER_MAX = 6 // <enum skip>
};

enum ThrusterFlag {
	TF_REVERSE = 1 << THRUSTER_REVERSE,
	TF_FORWARD = 1 << THRUSTER_FORWARD,
	TF_UP = 1 << THRUSTER_UP,
	TF_DOWN = 1 << THRUSTER_DOWN,
	TF_LEFT = 1 << THRUSTER_LEFT,
	TF_RIGHT = 1 << THRUSTER_RIGHT,
	TF_LINEAR = 1 << 6,
	TF_MAIN = 1 << 7
};

enum ThrusterType {
	THRTYPE_RCS,
	THRTYPE_MAIN,
	THRTYPE_MAX
};

using ThrusterArray = float[THRUSTER_MAX][THRTYPE_MAX];

inline ThrusterFlag ThrusterFlagFromDirection(vector3f dir)
{
	assert(dir.Length() > 0.001 && "Thruster direction vector is too short");
	assert(dir.Length() < 1e38 && "Thruster direction vector is too long");
	dir = dir.Normalized();
	const double crit = 0.577; // sqrt(0.333...) side of a cube with a diagonal of 1
	if (dir.z < -crit) return TF_REVERSE;
	else if (dir.z > crit) return TF_FORWARD;
	else if (dir.y > crit) return TF_UP;
	else if (dir.y < -crit) return TF_DOWN;
	else if (dir.x < -crit) return TF_LEFT;
	else if (dir.x > crit) return TF_RIGHT;
	else assert(false && "Impossible thruster direction vector");
	return TF_REVERSE;
}

#endif // THRUSTERCONFIG_H
