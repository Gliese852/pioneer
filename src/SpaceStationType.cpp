// Copyright © 2008-2024 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "SpaceStationType.h"
#include "FileSystem.h"
#include "JsonUtils.h"
#include "MathUtil.h"
#include "Pi.h"
#include "Ship.h"
#include "StringF.h"
#include "scenegraph/Tag.h"
#include "scenegraph/Model.h"
#include "utils.h"
#include "EnumStrings.h"

#include <algorithm>

static constexpr float INTERNAL_SPEED_LIMIT = 30; // m/s
static constexpr float EXTERNAL_SPEED_LIMIT = 300; // m/s

// TODO: Fix the horrible control flow that makes this exception type necessary.
struct StationTypeLoadError {};

std::vector<SpaceStationType> SpaceStationType::surfaceTypes;
std::vector<SpaceStationType> SpaceStationType::orbitalTypes;

SpaceStationType::SpaceStationType(const std::string &id_, const std::string &path_) :
	id(id_),
	model(0),
	modelName(""),
	angVel(0.f),
	dockMethod(SURFACE),
	numDockingPorts(0),
	lastDockStage(DockStage::DOCK_ANIMATION_NONE),
	lastUndockStage(DockStage::UNDOCK_ANIMATION_NONE),
	parkingDistance(0),
	parkingGapSize(0)
{
	Json data = JsonUtils::LoadJsonDataFile(path_);
	if (data.is_null()) {
		Output("couldn't read station def '%s'\n", path_.c_str());
		throw StationTypeLoadError();
	}

	modelName = data.value("model", "");

	const std::string type = data.value("type", "");
	if (type == "surface")
		dockMethod = SURFACE;
	else if (type == "orbital")
		dockMethod = ORBITAL;
	else {
		Output("couldn't parse station def '%s': unknown type '%s'\n", path_.c_str(), type.c_str());
		throw StationTypeLoadError();
	}

	angVel = data.value("angular_velocity", 0.0f);

	parkingDistance = data.value("parking_distance", 0.0f);
	parkingGapSize = data.value("parking_gap_size", 0.0f);

	padOffset = data.value("pad_offset", 150.f);

	model = Pi::FindModel(modelName, /* allowPlaceholder = */ false);
	if (!model) {
		Output("couldn't initialize station type '%s' because the corresponding model ('%s') could not be found.\n", path_.c_str(), modelName.c_str());
		throw StationTypeLoadError();
	}
	OnSetupComplete();
}

template <size_t S>
static const char *next_section(char (&dest)[S], const char* tail, char sep)
{
	const char *next = strchr(tail, sep);
	size_t len = next ? next - tail : strlen(tail);
	assert(len < S);
	len = std::min(len, S - 1);
	strncpy(dest, tail, len);
	dest[len] = 0;
	return tail + len + (next ? 1 : 0);
}

// https://forum.pioneerspacesim.net/viewtopic.php?f=3&t=669

static void waypoint_parse_sections(DockOperations::WayPoint &wp, const char* sections)
{
	char section[64];

	while (sections[0]) {
		sections = next_section(section, sections, ':');

		if (!strcmp(section, "pos")) {
			wp.flags |= DockOperations::WayPoint::Flag::ONLY_POS;
		} else if (!strcmp(section, "gate")) {
			wp.flags |= DockOperations::WayPoint::Flag::GATE;
		} else {
			// refs
			if (section[0] == '_') {
				strcpy(wp.out, section + 1);
				return;
			}
			sscanf(section,"%[^_]_%[^_]", wp.in, wp.out);
			if (strlen(wp.out) == 0 && strlen(wp.in) == strlen(section)) {
				strcpy(wp.out, wp.in);
			}
		}
	}
}

static void waypoint_extract_size_and_location(DockOperations::WayPoint &wp, const matrix4x4f &m)
{
	wp.loc = m;
	wp.loc.Renormalize();
	wp.radiusSqr = vector3f(m[0], m[4], m[8]).LengthSqr();
}

DockOperations::WayPoint waypoint_from_scenetag(SceneGraph::Tag *sceneTag)
{
	DockOperations::WayPoint wp{};

	char nameSection[64];
	const char *otherSections = next_section(nameSection, sceneTag->GetName().c_str(), ':');
	PiVerify(sscanf(nameSection, "wp_%s", wp.name) == 1);

	waypoint_extract_size_and_location(wp, sceneTag->GetGlobalTransform());
	waypoint_parse_sections(wp, otherSections);

	return wp;
}

SpaceStationType::Bay SpaceStationType::Bay::fromSceneTag(SceneGraph::Tag *sceneTag)
{
	Bay bay{};

	char baySection[64];
	const char *otherSections = next_section(baySection, sceneTag->GetName().c_str(), ':');
	PiVerify(sscanf(baySection, "pad_%[^_]_s%d_%d", bay.point.name, &bay.minShipSize, &bay.maxShipSize) == 3);

	waypoint_extract_size_and_location(bay.point, sceneTag->GetGlobalTransform());
	waypoint_parse_sections(bay.point, otherSections);

	bay.point.flags |= DockOperations::WayPoint::BAY;

	return bay;
}

void SpaceStationType::OnSetupComplete()
{
	// Since the model contains (almost) all of the docking information we have to extract that
	// and then generate any additional locators and information the station will need from it.

	// First we gather the MatrixTransforms that contain the location and orientation of the docking
	// locators/waypoints. We store some information within the name of these which needs parsing too.

	// Next we build the additional information required for docking ships with SPACE stations
	// on autopilot - this is the only option for docking with SPACE stations currently.
	// This mostly means offsetting from one locator to create the next in the sequence.

	using namespace DockOperations;

	// gather the tags
	std::vector<SceneGraph::Tag *> entrance_mts;
	std::vector<SceneGraph::Tag *> locator_mts;
	std::vector<SceneGraph::Tag *> exit_mts;
	std::vector<SceneGraph::Tag *> pad_mts;
	std::vector<SceneGraph::Tag *> waypoint_mts;
	model->FindTagsByStartOfName("entrance_", entrance_mts);
	model->FindTagsByStartOfName("loc_", locator_mts);
	model->FindTagsByStartOfName("exit_", exit_mts);
	model->FindTagsByStartOfName("pad_", pad_mts);
	model->FindTagsByStartOfName("wp_", waypoint_mts);

	// temporary structure for compatibility with legacy docking tags
	struct SPort {
		int portId;
		matrix4x4f m_approach[2];
	};

	std::vector<SPort> ports;

	Output("%s has:\n %lu entrances,\n %lu pads,\n %lu exits\n", modelName.c_str(), entrance_mts.size(), locator_mts.size(), exit_mts.size());

	std::sort(pad_mts.begin(), pad_mts.end(), [](const auto &a, const auto &b) { return a->GetName() < b->GetName(); });

	std::vector<WayPoint> waypoints;

	// new thing XXX
	if (pad_mts.size() > 0) {
		waypoints.reserve(waypoint_mts.size());
		for (auto sceneTag : waypoint_mts) {
			waypoints.push_back(waypoint_from_scenetag(sceneTag));
		}

		int bayID = 1;
		for (auto sceneTag : pad_mts) {
			auto bay = Bay::fromSceneTag(sceneTag);

			// approach route
			bay.approach.push_back(bay.point);
			char *prev = bay.point.in;
			while (prev[0]) {
				for (int i = 0; i < waypoints.size(); ++i) {
					auto &wp = waypoints[i];
					if (!strcmp(wp.name, prev)) {
						bay.approach.push_back(wp);
						prev = wp.in;
						break;
					} else {
						assert(i != waypoints.size() - 1 && "No waypoint with that name exists");
					}
				}
				assert(bay.approach.size() <= waypoints.size() && "It looks like there is a loop in the links");
			}


			if (bay.approach.size() == 1) {
				// add one by default, "above" the bay
				WayPoint wp{};
				wp.loc = bay.point.loc;
				wp.loc.Translate(0.f, 500.f, 0.f);
				wp.radiusSqr = 1.f;
				snprintf(wp.name, sizeof(wp.name), "%s-up", bay.point.name);
				bay.approach.push_back(wp);
			}

			bay.approach.back().flags |= WayPoint::APPROACH_START;

			// everything up to the gate is considered inside
			int speedLimit = INTERNAL_SPEED_LIMIT;
			for (auto &wp : bay.approach) {
				if (wp.flags & WayPoint::GATE) speedLimit = EXTERNAL_SPEED_LIMIT;
				wp.speed = speedLimit;
			}

			std::reverse(bay.approach.begin(), bay.approach.end());

			// departure route
			char *next = bay.point.out;
			while (next[0]) {
				for (int i = 0; i < waypoints.size(); ++i) {
					auto &wp = waypoints[i];
					if (!strcmp(wp.name, next)) {
						bay.departure.push_back(wp);
						next = wp.out;
						break;
					} else {
						assert(i != waypoints.size() - 1 && "No waypoint with that name exists");
					}
				}
				assert(bay.departure.size() <= waypoints.size() && "It looks like there is a loop in the links");
			}

			// everything up to the gate (inclusive) is considered inside
			// bay itself is not included in the departure route
			speedLimit = (bay.point.flags & WayPoint::GATE) ? EXTERNAL_SPEED_LIMIT : INTERNAL_SPEED_LIMIT;
			for (auto &wp : bay.departure) {
				wp.speed = speedLimit;
				if (wp.flags & WayPoint::GATE) speedLimit = EXTERNAL_SPEED_LIMIT;
			}

			bay.departure.back().flags |= WayPoint::ONLY_POS; // XXX ?

			bay.stages[DockStage::DOCKED] = bay.point.loc; // final (docked)
			lastDockStage = DockStage::DOCK_ANIMATION_NONE;
			lastUndockStage = DockStage::UNDOCK_ANIMATION_NONE;

			m_bays[bayID++] = bay; // XXX maybe not map?
		}
	}

	// Add the partially initialised ports
	for (SceneGraph::Tag *tag : entrance_mts) {
		int portId;
		PiVerify(1 == sscanf(tag->GetName().c_str(), "entrance_port%d", &portId));
		PiVerify(portId > 0);

		const matrix4x4f &trans = tag->GetGlobalTransform();

		SPort new_port;
		new_port.portId = portId;

		if (SURFACE == dockMethod) {
			const vector3f offDir = trans.Up().Normalized();
			new_port.m_approach[0] = trans;
			new_port.m_approach[0].SetTranslate(trans.GetTranslate() + (offDir * 500.0f));
		} else {
			const vector3f offDir = trans.Back().Normalized();
			new_port.m_approach[0] = trans;
			new_port.m_approach[0].SetTranslate(trans.GetTranslate() + (offDir * 1500.0f));
		}
		new_port.m_approach[1] = trans;
		new_port.m_approach[0].Renormalize();
		new_port.m_approach[1].Renormalize();
		ports.push_back(new_port);
	}

	for (auto locIter : locator_mts) {
		int bay, portId;
		int minSize, maxSize;
		char padname[8];
		matrix4x4f locTransform = locIter->GetGlobalTransform();
		locTransform.Renormalize();

		// eg:loc_A001_p01_s0_500_b01
		PiVerify(5 == sscanf(locIter->GetName().c_str(), "loc_%4s_p%d_s%d_%d_b%d", &padname[0], &portId, &minSize, &maxSize, &bay));
		PiVerify(bay > 0 && portId > 0);

		m_bays[bay].minShipSize = minSize;
		m_bays[bay].maxShipSize = maxSize;

		// find the port and setup the rest of it's information
#ifndef NDEBUG
		bool bFoundPort = false;
#endif
		matrix4x4f approach1(0.0);
		matrix4x4f approach2(0.0);
		for (auto &rPort : ports) {
			if (rPort.portId == portId) {
#ifndef NDEBUG
				bFoundPort = true;
#endif
				approach1 = rPort.m_approach[0];
				approach2 = rPort.m_approach[1];
				break;
			}
		}
		assert(bFoundPort);

		m_bays[bay].approach.push_back({ approach1, EXTERNAL_SPEED_LIMIT, 3.f, WayPoint::ONLY_POS | WayPoint::APPROACH_START });
		m_bays[bay].approach.push_back({ approach2, EXTERNAL_SPEED_LIMIT, 3.f, WayPoint::GATE });

		// now build the docking/leaving waypoints
		if (SURFACE == dockMethod) {
			// ground stations don't have leaving waypoints.
			m_bays[bay].stages[DockStage::DOCKED] = locTransform; // final (docked)
			m_bays[bay].approach.back().flags = WayPoint::BAY;
			lastDockStage = DockStage::DOCK_ANIMATION_NONE;
			lastUndockStage = DockStage::UNDOCK_ANIMATION_NONE;
		} else {
			struct TPointLine {
				// for reference: http://paulbourke.net/geometry/pointlineplane/
				static bool ClosestPointOnLine(const vector3f &Point, const vector3f &LineStart, const vector3f &LineEnd, vector3f &Intersection)
				{
					const float LineMag = (LineStart - LineEnd).Length();

					const float U = (((Point.x - LineStart.x) * (LineEnd.x - LineStart.x)) +
										((Point.y - LineStart.y) * (LineEnd.y - LineStart.y)) +
										((Point.z - LineStart.z) * (LineEnd.z - LineStart.z))) /
						(LineMag * LineMag);

					if (U < 0.0f || U > 1.0f)
						return false; // closest point does not fall within the line segment

					Intersection.x = LineStart.x + U * (LineEnd.x - LineStart.x);
					Intersection.y = LineStart.y + U * (LineEnd.y - LineStart.y);
					Intersection.z = LineStart.z + U * (LineEnd.z - LineStart.z);

					return true;
				}
			};

			// create the docking locators

			// above the pad
			vector3f intersectionPos(0.0f);
			const vector3f approach1Pos = approach1.GetTranslate();
			const vector3f approach2Pos = approach2.GetTranslate();
			{
				const vector3f p0 = locTransform.GetTranslate();			   // plane position
				const vector3f l = (approach2Pos - approach1Pos).Normalized(); // ray direction
				const vector3f l0 = approach1Pos + (l * 10000.0f);

				if (!TPointLine::ClosestPointOnLine(p0, approach1Pos, l0, intersectionPos)) {
					Output("No point found on line segment");
				}
			}
			matrix4x4f trans;
			trans = locTransform;
			trans.SetTranslate(intersectionPos);
			m_bays[bay].approach.push_back({ trans, INTERNAL_SPEED_LIMIT, 0.1f, WayPoint::BEFORE_BAY });
			// final (docked)
			m_bays[bay].approach.push_back({ locTransform, INTERNAL_SPEED_LIMIT, 0.1f, WayPoint::BAY });

			// all animation was replaced with real flight
			lastDockStage = DockStage::DOCK_ANIMATION_NONE;
			lastUndockStage = DockStage::UNDOCK_ANIMATION_NONE;

			m_bays[bay].stages[DockStage::DOCKED] = locTransform;

			// create the leaving locators

			matrix4x4f orient = locTransform.GetOrient();
			matrix4x4f EndOrient;

			if (exit_mts.empty()) {
				// leaving locators need to face in the opposite direction
				orient.RotateX(M_PI);
				orient.SetTranslate(locTransform.GetTranslate());
				EndOrient = approach2;
				EndOrient.SetRotationOnly(orient);
			} else {
				// leaving locators, use whatever orientation they have
				orient.SetTranslate(locTransform.GetTranslate());
				int exitport = 0;
				for (auto &exitIt : exit_mts) {
					PiVerify(1 == sscanf(exitIt->GetName().c_str(), "exit_port%d", &exitport));
					if (exitport == portId) {
						EndOrient = exitIt->GetGlobalTransform();
						break;
					}
				}
				if (exitport == 0) {
					EndOrient = approach2;
				}
			}

			// above the pad
			trans = locTransform;
			trans.SetTranslate(intersectionPos);
			m_bays[bay].departure.push_back({ trans, INTERNAL_SPEED_LIMIT, 0.1f });

			// exit
			m_bays[bay].departure.push_back({ EndOrient, INTERNAL_SPEED_LIMIT, 0.1f, WayPoint::ONLY_POS });
		}
	}

	numDockingPorts = m_bays.size();

	assert(!m_bays.empty());
}

bool SpaceStationType::GetShipApproachWaypoints(const unsigned int port, Uint32 stage, positionOrient_t &outPosOrient) const
{
	auto bay = GetBay(port);

	if (stage >= bay.approach.size()) return false;

	const matrix4x4f &mt = bay.approach[stage].loc;
	outPosOrient.pos = vector3d(mt.GetTranslate());
	outPosOrient.xaxis = vector3d(mt.GetOrient().VectorX());
	outPosOrient.yaxis = vector3d(mt.GetOrient().VectorY());
	outPosOrient.zaxis = vector3d(mt.GetOrient().VectorZ());
	outPosOrient.xaxis = outPosOrient.xaxis.Normalized();
	outPosOrient.yaxis = outPosOrient.yaxis.Normalized();
	outPosOrient.zaxis = outPosOrient.zaxis.Normalized();

	return true;
}
/*static*/
void SpaceStationType::Init()
{
	PROFILE_SCOPED()
	static bool isInitted = false;
	if (isInitted)
		return;
	isInitted = true;

	// load all station definitions
	namespace fs = FileSystem;
	for (fs::FileEnumerator files(fs::gameDataFiles, "stations", 0); !files.Finished(); files.Next()) {
		const fs::FileInfo &info = files.Current();
		if (ends_with_ci(info.GetPath(), ".json")) {
			const std::string id(info.GetName().substr(0, info.GetName().size() - 5));
			try {
				SpaceStationType st = SpaceStationType(id, info.GetPath());
				switch (st.dockMethod) {
				case SURFACE: surfaceTypes.push_back(st); break;
				case ORBITAL: orbitalTypes.push_back(st); break;
				}
			} catch (StationTypeLoadError) {
				// TODO: Actual error handling would be nice.
				Error("Error while loading Space Station data (check stdout/output.txt).\n");
			}
		}
	}
}

/*static*/
const SpaceStationType *SpaceStationType::RandomStationType(Random &random, const bool bIsGround)
{
	if (bIsGround) {
		return &surfaceTypes[random.Int32(SpaceStationType::surfaceTypes.size())];
	}

	return &orbitalTypes[random.Int32(SpaceStationType::orbitalTypes.size())];
}

/*static*/
const SpaceStationType *SpaceStationType::FindByName(const std::string &name)
{
	for (auto &sst : surfaceTypes)
		if (sst.id == name)
			return &sst;
	for (auto &sst : orbitalTypes)
		if (sst.id == name)
			return &sst;
	return nullptr;
}

DockStage SpaceStationType::PivotStage(DockStage s) const {
	switch (s) {
		// at these stages, the position of the ship relative to the station has
		// already been calculated and is in the shipDocking_t data
		case DockStage::TOUCHDOWN:
		case DockStage::JUST_DOCK:
		case DockStage::LEVELING:
		case DockStage::REPOSITION:
			return DockStage::MANUAL;
		// at these stages, the station does not control the position of the ship
		case DockStage::CLEARANCE_GRANTED:
		case DockStage::APPROACH:
		case DockStage::LEAVE:
		case DockStage::DEPARTURE:
			return DockStage::NONE;
		default: return s;
	}
}

const char *SpaceStationType::DockStageName(DockStage s) const {
	return EnumStrings::GetString("DockStage", int(s));
}

matrix4x4f SpaceStationType::GetStageTransform(int bay, DockStage stage) const
{
	return m_bays.at(bay + 1).stages.at(stage);
}
