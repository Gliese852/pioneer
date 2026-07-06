local Engine = require 'Engine'

local Headless = {}

function Headless.StartGame()
	SystemPath = require 'SystemPath'
	Game = require 'Game'
	location = SystemPath.New(0,0,0,0,18)
	startTime = util.standardGameStartTime()
	shipType = 'pumpkinseed'
	Game.StartGame(location, startTime, shipType)
	Game.SetRealtimeMode(true)
	return "COOL?"
end

function Headless.EndGame()
	local Game = require 'Game'
	Game.EndGame()
end

function Headless.Quit()
	Engine.Quit()
end

return Headless
