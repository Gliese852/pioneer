// Copyright © 2008-2021 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "DemoPowerSystem.h"
#include "scenegraph/FindNodeVisitor.h"
#include "scenegraph/MatrixTransform.h"
#include "scenegraph/Thruster.h"

DemoPowerSystem::DemoPowerSystem(SceneGraph::Model *m)
{
	// collect the IDs of the trusters that will burn
	bool hasMainThruster = false;
	// get thrusters group
	SceneGraph::FindNodeVisitor thrusterFinder(SceneGraph::FindNodeVisitor::MATCH_NAME_FULL, "thrusters");
	m->GetRoot()->Accept(thrusterFinder);
	auto &results = thrusterFinder.GetResults();
	auto *thrusters = static_cast<SceneGraph::Group *>(results.at(0));

	// looking for backward directed thrusters
	for (unsigned i = 0; i < thrusters->GetNumChildren(); i++) {
		auto *mt = static_cast<SceneGraph::MatrixTransform *>(thrusters->GetChildAt(i));
		auto direction = mt->GetTransform().GetOrient() * vector3f(0.f, 0.f, 1.f);
		auto *my_thruster = static_cast<SceneGraph::Thruster *>(mt->GetChildAt(0));
		if (ThrusterConfig::ThrusterFromDirection(direction) == THRUSTER_FORWARD) {
			auto type = my_thruster->GetConfig().type;
			hasMainThruster = hasMainThruster || type == ThrusterConfig::TYPE_MAIN;
			fwdThrusters[my_thruster->GetID()] = type;
		}
	}
	// if there are main engines, delete all others
	if (hasMainThruster) {
		for(auto i = fwdThrusters.begin(); i != fwdThrusters.end(); ) {
			if (i->second != ThrusterConfig::TYPE_MAIN) {
				i = fwdThrusters.erase(i);
			} else {
				++i;
			}
		}
	}
}

float DemoPowerSystem::GetLevel(unsigned thrusterID) {
	if (fwdThrusters.find(thrusterID) != fwdThrusters.end()) {
		return rng.Double_closed(0.6, 0.8); 
	} else {
		return 0.0f;
	}
}
