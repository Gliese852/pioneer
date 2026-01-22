#include "Space.h"

#include "doctest.h"

struct Player;

struct Galaxy : public RefCounted {
};

struct Game {

	enum TimeAccel {
		TIMEACCEL_PAUSED,
		TIMEACCEL_1X,
		TIMEACCEL_10X,
		TIMEACCEL_100X,
		TIMEACCEL_1000X,
		TIMEACCEL_10000X,
		TIMEACCEL_HYPERSPACE,
		TIMEACCEL_MAX
	};

	double GetTime() const { return m_time; }
	float GetTimeStep() const { return m_timeStep; }
	TimeAccel GetTimeAccel() const { return m_timeAccel; }
	Player *GetPlayer() const { return m_player; }
	RefCountedPtr<Galaxy> GetGalaxy() const { return m_galaxy; }
	bool IsNormalSpace() const { return m_isNormalSpace; }

	double m_time;
	float m_timeStep;
	TimeAccel m_timeAccel;
	Player *m_player;
	RefCountedPtr<Galaxy> m_galaxy;
	bool m_isNormalSpace;
};

TEST_CASE("Ship crash")
{
	RefCountedPtr<Galaxy> galaxy;
	Game game;
	game.m_galaxy = galaxy;
	Space space(&game, galaxy);
	MESSAGE("HELLO AGAIN");
}
