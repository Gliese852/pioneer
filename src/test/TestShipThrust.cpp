#include "Game.h"
#include "Space.h"
#include "FileSystem.h"
#include "lua/Lua.h"
#include "lua/LuaEvent.h"
#include "pigui/LuaPiGui.h"
#include "Pi.h"
#include "BaseSphere.h"
#include "graphics/Graphics.h"
#include "graphics/dummy/RendererDummy.h"
#include "core/OS.h"
#include "SpaceStationType.h"
#include "ModelCache.h"
#include "NavLights.h"
#include "Shields.h"
#include "Body.h"
#include "Ship.h"
#include "Frame.h"

#include "doctest.h"

static std::basic_ostream<char, std::char_traits<char>> &operator<<(std::basic_ostream<char, std::char_traits<char>> &log, const vector3d &v)
{
	log << v.x << " " << v.y << " " << v.z;
	return log;
}

static std::basic_ostream<char, std::char_traits<char>> &operator<<(std::basic_ostream<char, std::char_traits<char>> &log, const matrix3x3d &m)
{
	log << m.VectorX() << std::endl << m.VectorY() << std::endl << m.VectorZ();
	return log;
}

static void log_ship(Ship *s)
{
	std::cout << std::endl << "THE SHIP";
	auto f = Frame::GetFrame(s->GetFrame());
	auto fb = f->GetBody();
	auto fsb = f->GetSystemBody();

	if (fb) {
		std::cout << " fb: " << fb->GetLabel();
	} else {
		std::cout << " fb: NO";
	}

	if (fsb) {
		std::cout << " fsb: " << fsb->GetName();
	}

	std::cout << " pos: " << s->GetPosition() << " vel: " << s->GetVelocity()
		<< std::endl << " ori: " << std::endl << s->GetOrient();

	std::cout << std::endl;
}


TEST_CASE("ship_thrust")
{
	std::map<std::string, std::string> options;
	Pi::Init(options, true);
	Lua::Init(Pi::GetAsyncJobQueue());
	PiGui::Lua::Init();
	Lua::InitModules();
	LuaEvent::Init();
	FileSystem::Init();
	BaseSphere::Init(Pi::renderer);
	Pi::modelCache = new ModelCache(Pi::renderer);
	Shields::Init(Pi::renderer);
	NavLights::Init(Pi::renderer);
	SpaceStationType::Init();
	assert(Pi::modelCache);
	SystemPath path{ -2, -4, -1, 0, 1 };
	Game g(path, 0);

	Ship *s = nullptr;

	std::cout << "BODIES: " << g.GetSpace()->GetNumBodies() << std::endl;
	auto bodies = g.GetSpace()->GetBodies();
	for(auto it = bodies.begin(); it != bodies.end(); ++it) {
		std::cout
			<< "body: " << (*it)->GetLabel()
			<< " pos: " << (*it)->GetPosition()
			<< std::endl;

		if ((*it)->IsType(ObjectType::SHIP)) {
			s = static_cast<Ship*>(*it);
		}
	}

	assert(s);

	s->SetFrame(0);
	s->SetPosition({ 0, 0, 0 });
	s->SetVelocity({ 0, 0, 0 });

	// auto ori = s->GetOrient();
	s->SetOrient(matrix3x3d::RotateX(DEG2RAD(45.0)));

	auto prop = s->GetPropulsion();
	vector3d upVel{ 0, 100'000, 0 };
	g.SetTimeAccel(Game::TIMEACCEL_1X);

	for (int i = 0; i < 100; ++i) {
		prop->AIMatchVel(upVel);
		g.TimeStep(g.GetTimeStep());
		log_ship(s);
	}
}
