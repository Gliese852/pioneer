#include "Pi.h"
#include "lua/Lua.h"
#include "pigui/LuaPiGui.h"
#include "core/GuiApplication.h"
#include "SaveGameManager.h"
#include "BaseSphere.h"
#include "Json.h"
#include "Game.h"
#include "core/StringUtils.h"
#include "pigui/PiGui.h"
#include "lua/LuaEvent.h"

#include "doctest.h"

class TestLoop : public Application::Lifecycle {
	void Start() override
	{
		m_counter = 0;
		std::cout << "STARTING TESTLOOP" << std::endl;
		Json rootNode = SaveGameManager::LoadGameToJson("test_crash");
		CHECK(rootNode.is_object());

		Pi::game = new Game(rootNode);
		Pi::game->SetTimeAccel(Game::TIMEACCEL_10000X);
		LuaEvent::Queue("onGameStart");
		LuaEvent::Emit();

	}
	void Update(float deltaTime) override
	{
		++m_counter;
		auto step = Pi::game->GetTimeStep();
		std::cout << "COUNTER:" << m_counter << " TIME: " << format_date(Pi::game->GetTime()) << " TIMESTEP: " << step << std::endl;
		if (m_counter > 100) {
			RequestEndLifecycle();
		}
		Pi::game->TimeStep(step);
		BaseSphere::UpdateAllBaseSphereDerivatives();
		Pi::pigui->NewFrame();
		PiGui::EmitEvents();
		PiGui::RunHandler(deltaTime, "game");
		Pi::pigui->Render();

	}
	void End() override
	{
		std::cout << "END TESTLOOP" << std::endl;
	}

	int m_counter;
};

TEST_CASE("ship_crash")
{
	RefCountedPtr<Application::Lifecycle> testLoop;
	testLoop.Reset(new TestLoop());
	std::map<std::string, std::string> options;

	Pi::Init(options, true);

	std::cout << "QUEUE TESTLOOP\n";
	Pi::GetApp()->QueueLifecycle(testLoop);

	Pi::GetApp()->Run();
}
