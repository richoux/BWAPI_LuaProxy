local socket = require('socket')
local server = socket.tcp()
server:bind('*', 13337)
server:listen(32)
local client = server:accept()

local mp = require 'MessagePack'
local ip, port = server:getsockname()
print(ip)
print(port)

--onStart data
local selfID
local players = {}
local playerRace = {}
local ally = {}
local mapName
local mapWidth
local mapHeight

--onFrame data
local myResearch = {}
local myUpgrades = {}
local enemyResearch = {}
local enemyUpgrades = {}
local unitID = {}
local unitPlayerID = {}
local unitType = {} -- key=ID, value=type
local unitMapType = {} -- key=type, value=table of IDs
local unitPosX = {}
local unitPosY = {}
local unitHP = {}
local unitInitHP = {}
local unitShield = {}
local unitEnergy = {}
local unitOrderID = {}
local unitSpiderMineCount = {}
local unitScarabCount = {}
local unitInterceptorCount = {}
local unitVelocityX = {}
local unitVelocityY = {}

local unitIsAccelerating  = {}
local unitIsAttackFrame  = {}
local unitIsAttacking  = {}
local unitIsBeingHealed  = {}
local unitIsBlind  = {}
local unitIsBraking  = {}
local unitIsBurrowed  = {}
local unitIsCloaked  = {}
local unitIsDefenseMatrixed = {}
local unitIsDetected  = {}
local unitIsEnsnared = {}
local unitIsFlying = {}
local unitIsFollowing = {}
local unitIsHallucination  = {}
local unitIsHoldingPosition = {}
local unitIsIdle  = {}
local unitIsInterruptible  = {}
local unitIsInvincible  = {}
local unitIsIrradiated = {}
local unitIsLoaded = {}
local unitIsLockedDown = {}
local unitIsMaelstrommed = {}
local unitIsMorphing  = {}
local unitIsMoving  = {}
local unitIsParasited  = {}
local unitIsPatrolling = {}
local unitIsPlagued = {}
local unitIsRepairing = {}
local unitIsSelected  = {}
local unitIsSieged = {}
local unitIsStartingAttack  = {}
local unitIsStasised = {}
local unitIsStimmed = {}
local unitIsStuck  = {}
local unitIsTargetable  = {}
local unitIsUnderAttack  = {}
local unitIsUnderDarkSwarm  = {}
local unitIsUnderDisruptionWeb  = {}
local unitIsUnderStorm  = {}
local unitIsVisible = {}


-- shifts
local x_left_shift = 0
local y_top_shift = 0
local x_right_shift = 0
local y_bottom_shift = 0

local gameState = torch.Tensor()

while true do

   local mpac, status, partial = client:receive("*a")

   if( mpac ~= nil ) then 
      for i, command in mp.unpacker(mpac) do
	 loadstring(command)();
      end
   else
      print("mpac nil");
   end
   if( status ~= nil ) then 
      print("status " .. status);
   end

   if( x_left_shift > 0 ) then 
      gameState[{{1,x_left_shift},{1,gameState:size(2)},1}] = 3

   if( y_top_shift > 0 ) then 
      gameState[{{1,gameState:size(1)},{1,y_top_shift},1}] = 3

   if( x_right_shift > 0 ) then 
      gameState[{{x_right_shift,gameState:size(1)},{1,gameState:size(2)},1}] = 3

   if( y_bottom_shift > 0 ) then 
      gameState[{{1,gameState:size(1)},{y_bottom_shift,gameState:size(2)},1}] = 3
   
   
   if status == "closed" then break end
   client:send("0\n")
end

client:close()

-- commands
function attack(unit, x, y)
   client:send("1," .. unit .. "," .. x .. "," .. y .. ",0")
end

function attack(unit, target)
   client:send("2," .. unit .. "," .. target .. ",0" .. ",0")
end

function rightClick(unit, x, y)
   client:send("3,".. unit .. "," .. x .. "," .. y .. ",0")
end

function rightClick(unit, target)
   client:send("4," .. unit .. "," .. target .. ",0" .. ",0")
end

function stop(unit)
   client:send("10," .. unit .. ",0" .. ",0" .. ",0")
end

function holdPosition(unit)
   client:send("11," .. unit .. ",0" .. ",0" .. ",0")
end

function patrol(unit, x, y)
   client:send("12,".. unit .. "," .. x .. "," .. y .. ",0")
end

function follow(unit, target)
   client:send("13," .. unit .. "," .. target .. ",0" .. ",0")
end

function repair(unit, target)
   client:send("16," .. unit .. "," .. target .. ",0" .. ",0")
end

function burrow(unit)
   client:send("18," .. unit .. ",0" .. ",0" .. ",0")
end

function unburrow(unit)
   client:send("19," .. unit .. ",0" .. ",0" .. ",0")
end

function siege(unit)
   client:send("20," .. unit .. ",0" .. ",0" .. ",0")
end

function unsiege(unit)
   client:send("21," .. unit .. ",0" .. ",0" .. ",0")
end

function cloak(unit)
   client:send("22," .. unit .. ",0" .. ",0" .. ",0")
end

function decloak(unit)
   client:send("23," .. unit .. ",0" .. ",0" .. ",0")
end

function loadUnit(unit, target)
   client:send("26," .. unit .. "," .. target .. ",0" .. ",0")
end

function unloadUnit(unit, target)
   client:send("27," .. unit .. "," .. target .. ",0" .. ",0")
end

function unloadAll(unit)
   client:send("28," .. unit .. ",0" .. ",0" .. ",0")
end

function unloadAll(unit, x, y)
   client:send("29,".. unit .. "," .. x .. "," .. y .. ",0")
end

function useTech(unit, techType)
   client:send("38," .. unit .. "," .. techType .. ",0" .. ",0")
end

function useTech(unit, techType, x, y)
   client:send("39," .. unit .. "," .. techType .. "," .. x .. "," .. y)
end

function useTech(unit, techType, target)
   client:send("40," .. unit .. "," .. techType .. "," .. target .. ",0")
end

