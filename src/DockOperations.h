// Copyright © 2008-2024 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#pragma once

#include "matrix4x4.h"

namespace SceneGraph {
	class Tag;
}

namespace DockOperations {

	using Name = char[16];

	struct WayPoint {

		enum Flag : uint64_t {
			BAY            = 0x01,
			BEFORE_BAY     = 0x02,
			ONLY_POS       = 0x04,
			GATE           = 0x08,
			EXTERNAL       = 0x10,
			APPROACH_START = 0x20,

			INVALID = 0x8000, // XXX use?
		};

		matrix4x4f loc;

		float speed;
		float radiusSqr;
		uint64_t flags;

		Name name;
		Name in;
		Name out;
	};

	struct Command {

		enum class Type {
			FLY_TO,
			BYE,
		};

		Type type;
		WayPoint *waypoint;
		WayPoint *waypointAfter;
	};
};
