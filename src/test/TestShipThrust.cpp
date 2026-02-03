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
#include "ship/PlayerShipController.h"
#include "ShipAICmd.h"
#include "CargoBody.h"

#include <iomanip>

#include "doctest.h"

static std::basic_ostream<char, std::char_traits<char>> &operator<<(std::basic_ostream<char, std::char_traits<char>> &log, const vector3d &v)
{
	log << v.x << " " << v.y << " " << v.z;
	return log;
}

static std::basic_ostream<char, std::char_traits<char>> &operator<<(std::basic_ostream<char, std::char_traits<char>> &log, const matrix3x3d &m)
{
	log << m.VectorX() << " | " << m.VectorY() << " | " << m.VectorZ();
	return log;
}

class NullAICommand : public AICommand {
public:
	NullAICommand(DynamicBody *db) : AICommand(db, AICommand::CMD_NONE) {}
private:
	bool TimeStepUpdate(float timeStep) override {
		return false;
	}
};

static void ship_log(Game *g, Ship *s)
{
	std::cout << g->GetTime();
	auto f = Frame::GetFrame(s->GetFrame());
	auto fb = f->GetBody();
	auto fsb = f->GetSystemBody();
	auto prop = s->GetPropulsion();

	if (fb) {
		std::cout << " fb: " << fb->GetLabel();
	} else {
		std::cout << " fb: NO";
	}

	if (fsb) {
		std::cout << " fsb: " << fsb->GetName();
	}

	std::cout << " thr: " << prop->GetLinThrusterState();

	std::cout << " pos: " << s->GetPosition() << " vel: " << s->GetVelocity()
		<< " ori: " << s->GetOrient();
	std::cout << " grav: " << s->GetGravityForce();

	std::cout << std::endl;
}

void ship_thrust_there(Ship *m_dBody, const vector3d &updir)
{
	auto m_prop = m_dBody->GetPropulsion();

	vector3d thrustDir = updir * m_dBody->GetOrient();
	vector3d maxThrust = m_prop->GetThrust(thrustDir);
	vector3d thrust{ thrustDir.x / maxThrust.x, thrustDir.y / maxThrust.y, thrustDir.z / maxThrust.z };
	double invScale = std::max(abs(thrust.x),
							   std::max(abs(thrust.y),
										abs(thrust.z)));
	thrust /= invScale;


	m_prop->SetLinThrusterState(thrust);
	auto end_thr = m_prop->GetLinThrusterState();
	std::cout << " hm, thrust: " << end_thr << " - ";
}


TEST_CASE("ship_thrust")
{
	std::map<std::string, std::string> options;
	Pi::Init(options, true);
	ShipType::Init();
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

	g.TimeStep(g.GetTimeStep());

	s->SetFrame(0);
	s->SetPosition({ 0, 0, 0 });
	s->SetVelocity({ 0, 0, 0 });

	if (s->GetController()->GetType() == ShipController::PLAYER) {
		auto psc = static_cast<PlayerShipController*>(s->GetController());
		psc->SetLowThrustPower(0);
	}

	s->SetAICommand(new NullAICommand(s));

	// auto ori = s->GetOrient();
	s->SetOrient(matrix3x3d::RotateX(DEG2RAD(60.0)));

	auto prop = s->GetPropulsion();
	vector3d upVel{ 0, -100'000, 0 };

	auto l = Lua::manager->GetLuaState();

	g.SetTimeAccel(Game::TIMEACCEL_1X);

	LuaEvent::Queue("onGameStart");
	LuaEvent::Emit();

	std::cout << std::endl
		<< std::setprecision(4) << std::fixed;


	for (int i = 0; i < 200; ++i) {

		ship_thrust_there(s, upVel);
		//prop->AIMatchVel(upVel);
		// prop->AIFaceDirection(upVel);
		g.TimeStep(g.GetTimeStep());
		ship_log(&g, s);
	}
}
