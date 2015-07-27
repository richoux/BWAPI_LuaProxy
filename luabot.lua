local zmq = require 'lzmq'
local context = zmq.context()
local msgpack = require 'MessagePack'
paths.dofile('commands.lua')

local hostname = '192.168.1.11'
local port = 5555

print('connecting to: ' .. hostname .. ':' .. port)

local sock, err = context:socket{zmq.REQ,
				 connect = 'tcp://' .. hostname .. ':' .. port}

sock:send_all({'hiiiiiiiiiiiiiiiii from Torch'})
print('Connected. Receiving game state. Establishing command control.')

----------------------------------------------------------------------
-- initializing state with empty tables
local gs = paths.dofile('gameState.lua')
--onStart data
local selfID = 'uninitialized'
local players = {}
local playerRace = {}
local ally = {}
local mapName = 'uninitialized'
local mapWidth = 'uninitialized'
local mapHeight = 'uninitialized'

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
----------------------------------------------------------------------

local counter = 1

while true do
   local msg = table.concat(sock:recv_all())
   for i, command in msgpack.unpacker(msg) do
      loadstring(command)
      -- print(command)
   end

   print('Frame ' .. counter)

   print("Self ID: " .. selfID);
   for p in ipairs(players) do
      print("Player ID: " .. p);
      print("Player race: " .. playerRace[p]);
      print("Player " .. p .. " is an ally: " .. ally[p]);
   end

   print("Map name: " .. mapName);
   print("Map Width: " .. mapWidth);
   print("Map Height: " .. mapHeight);

   print('')

   counter = counter + 1
   sock:send_all({""})
end
