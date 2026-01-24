#include "Pi.h"
#include "lua/Lua.h"
#include "pigui/LuaPiGui.h"
#include "core/GuiApplication.h"
#include "SaveGameManager.h"
#include "Json.h"
#include "Game.h"

#include "doctest.h"

class TestLoop : public Application::Lifecycle {
	void Start() override
	{
		m_counter = 0;
		std::cout << "STARTING TESTLOOP" << std::endl;
		Json rootNode = SaveGameManager::LoadGameToJson("test_crash");
		CHECK(rootNode.is_object());

		Pi::game = new Game(rootNode);

	}
	void Update(float deltaTime) override
	{
		++m_counter;
		std::cout << "COUNTER:" << m_counter << std::endl;
		if (m_counter > 500) {
			RequestEndLifecycle();
		}
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
