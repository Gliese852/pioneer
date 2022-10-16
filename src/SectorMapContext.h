// Copyright © 2008-2022 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#pragma once

#include "galaxy/StarSystem.h"
#include "galaxy/Galaxy.h"
#include "GameConfig.h"
#include "graphics/Renderer.h"
#include "Input.h"

struct SectorMapContext {

	using DisplayMode = unsigned;

	enum DisplayModes : DisplayMode {
		DEFAULT =    1 << 0,
		ALWAYS =     1 << 1,
		HIDE_LABEL = 1 << 2
	};

	RefCountedPtr<Galaxy> galaxy;
	Input::Manager *input;
	GameConfig *config;
	Graphics::Renderer *renderer;
	std::function<void(const SystemPath &path)> onClickLabel;
	std::function<DisplayMode(const SystemPath &path)> systemDisplayMode;
};
