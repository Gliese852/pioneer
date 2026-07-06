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
end

function Headless.LoadGame(filename)
	local Game = require 'Game'
	Game.LoadGame(filename)
	Game.SetRealtimeMode(true)
end

function Headless.EndGame()
	local Game = require 'Game'
	Game.EndGame()
end

function Headless.Quit()
	Engine.Quit()
end

function Headless.Perf()
	return Engine.GetPerfString()
end

function Headless.Profile()
	return Engine.RequestProfileFrame()
end

function Headless.Status()
	local Game = require 'Game'

	local year, month, day, hour, minute, second = Game.GetDateTime()
	local time = string.format("%04i-%02i-%02i - %02i:%02i:%02i", year, month, day, hour, minute, second)

	local s = "TIME: " .. time .. " PERF: " .. Headless.Perf()
	s = s .. "\nAI: " .. Game.player:GetAIStatusText()

	return s
end

function Headless.Reload()
	return package.reimport('Headless')
end

return Headless
