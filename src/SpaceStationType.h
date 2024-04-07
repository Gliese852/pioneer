// Copyright © 2008-2024 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _SPACESTATIONTYPE_H
#define _SPACESTATIONTYPE_H

#include "Random.h"
#include "matrix4x4.h"
#include "vector3.h"
#include "DockOperations.h"

#include <map>
#include <vector>
#include <string>

//Space station definition, loaded from data/stations

class Ship;
namespace SceneGraph {
	class Model;
	class Tag;
}

enum class DockStage { // <enum scope='DockStage' name=DockStage public>

	NONE,
	MANUAL,

	DOCK_STAGES_BEGIN,

	CLEARANCE_GRANTED,

	APPROACH,

	DOCK_ANIMATION_NONE,
	DOCK_ANIMATION_1,
	DOCK_ANIMATION_2,
	DOCK_ANIMATION_3,
	DOCK_ANIMATION_MAX,

	TOUCHDOWN,
	LEVELING,
	REPOSITION,
	JUST_DOCK,

	DOCK_STAGES_END,

	DOCKED,

	UNDOCK_STAGES_BEGIN,

	UNDOCK_BEGIN,

	UNDOCK_ANIMATION_NONE,
	UNDOCK_ANIMATION_1,
	UNDOCK_ANIMATION_2,
	UNDOCK_ANIMATION_3,
	UNDOCK_ANIMATION_MAX,

	UNDOCK_END,

	DEPARTURE,

	LEAVE,

	UNDOCK_STAGES_END,
};

class SpaceStationType {
public:

	struct Bay {

		int minShipSize;
		int maxShipSize;
		bool inUse;

		DockOperations::WayPoint point;

		std::map<DockStage, matrix4x4f> stages;
		std::vector<DockOperations::WayPoint> approach;
		std::vector<DockOperations::WayPoint> departure;

		static Bay fromSceneTag(SceneGraph::Tag *sceneTag);
	};

	struct positionOrient_t {
		vector3d pos;
		vector3d xaxis;
		vector3d yaxis;
		vector3d zaxis;
	};

	DockStage PivotStage(DockStage s) const;

private:
	std::string id;
	SceneGraph::Model *model;
	std::string modelName;
	float angVel;
	enum DOCKMETHOD { SURFACE,
		ORBITAL } dockMethod;
	unsigned int numDockingPorts;
	DockStage lastDockStage;
	DockStage lastUndockStage;
	float parkingDistance;
	float parkingGapSize;
	std::map<Uint32, Bay> m_bays;
	float padOffset;

	static std::vector<SpaceStationType> surfaceTypes;
	static std::vector<SpaceStationType> orbitalTypes;

public:
	SpaceStationType(const std::string &id, const std::string &path);

	static bool IsDockStage(DockStage s) {
		return
			int(s) > int(DockStage::DOCK_STAGES_BEGIN) &&
			int(s) < int(DockStage::DOCK_STAGES_END);
	}

	static bool IsUndockStage(DockStage s) {
		return
			int(s) > int(DockStage::UNDOCK_STAGES_BEGIN) &&
			int(s) < int(DockStage::UNDOCK_STAGES_END);
	}

	static DockStage NextAnimStage(DockStage s) {
		return DockStage(int(s) + 1);
	}

	const char *DockStageName(DockStage s) const;

	void OnSetupComplete();

	// Call functions in the station .lua
	bool GetShipApproachWaypoints(const unsigned int port, Uint32 stage, positionOrient_t &outPosOrient) const;

	matrix4x4f GetStageTransform(int bay, DockStage stage) const;
	const Bay &GetBay(int bayID) const { return m_bays.at(bayID + 1); }

	const std::string &ModelName() const { return modelName; }
	float AngVel() const { return angVel; }
	bool IsSurfaceStation() const { return (SURFACE == dockMethod); }
	bool IsOrbitalStation() const { return (ORBITAL == dockMethod); }
	unsigned int NumDockingPorts() const { return numDockingPorts; }
	int NumDockingStages() const { return int(lastDockStage) - int(DockStage::DOCK_ANIMATION_NONE); }
	int NumUndockStages() const { return int(lastUndockStage) - int(DockStage::UNDOCK_ANIMATION_NONE); }
	DockStage LastDockStage() const { return lastDockStage; }
	DockStage LastUndockStage() const { return lastUndockStage; }
	float ParkingDistance() const { return parkingDistance; }
	float ParkingGapSize() const { return parkingGapSize; }

	static void Init();

	static const SpaceStationType *RandomStationType(Random &random, const bool bIsGround);
	static const SpaceStationType *FindByName(const std::string &name);
};

#endif
