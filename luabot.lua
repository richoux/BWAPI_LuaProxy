local socket = require('socket')
local server = assert(socket.bind('*', 13337))
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

local client = server:accept()

while true do

   local mpac, status, partial = client:receive("*a")

   if( mpac ~= nil ) then 
      print("mpac " .. mpac);
      for i, command in mp.unpacker(mpac) do
	 print("i=" .. i .. ", command=" .. command);
	 loadstring(command)(); 
      end
   else
      print("mpac nil");
   end
   if( status ~= nil ) then 
      print("status " .. status);
   end
   if( partial ~= nil ) then 
      print("partial " .. partial);
      for i, v in mp.unpacker(partial) do
	 print("i=" .. i .. ", v=" .. v);	 
      end
   end

   print("Self ID: " .. selfID);
   for p in players do
      print("Player ID: " .. p);
      print("Player race: " .. playerRace[p]);
      print("Player " .. p .. " is an ally: " .. ally[p]);
   end

   print("Map name: " .. mapName);
   print("Map Width: " .. mapWidth);
   print("Map Height: " .. mapHeight);

   local data = {};
   for i, v in mp.unpacker(mpac) do
      data[i] = v;
      --print("i=" .. i .. ", v=" .. v);
   end

   io.write("Minerals: " );
   print(data[1]);
   
   io.write("Gas: " );
   print(data[2]);
   
   io.write("Supply: " );
   if( data[4] ~= nil ) then
      print(data[3] .. "/" .. data[4]);
   else
      print(data[3]);
   end
   
   if status == "closed" then break end
   client:send("0\n")
end

client:close()

-- commands
function attack(unit, x, y)
   client:send(":1," .. unit .. "," .. x .. "," .. y .. ",0")
end

function attack(unit, target)
   client:send(":2," .. unit .. "," .. target .. ",0" .. ",0")
end

function rightClick(unit, x, y)
   client:send(":3,".. unit .. "," .. x .. "," .. y .. ",0")
end

function rightClick(unit, target)
   client:send(":4," .. unit .. "," .. target .. ",0" .. ",0")
end

function stop(unit)
   client:send(":10," .. unit .. ",0" .. ",0" .. ",0")
end

function holdPosition(unit)
   client:send(":11," .. unit .. ",0" .. ",0" .. ",0")
end

function patrol(unit, x, y)
   client:send(":12,".. unit .. "," .. x .. "," .. y .. ",0")
end

function follow(unit, target)
   client:send(":13," .. unit .. "," .. target .. ",0" .. ",0")
end

function repair(unit, target)
   client:send(":16," .. unit .. "," .. target .. ",0" .. ",0")
end

function burrow(unit)
   client:send(":18," .. unit .. ",0" .. ",0" .. ",0")
end

function unburrow(unit)
   client:send(":19," .. unit .. ",0" .. ",0" .. ",0")
end

function siege(unit)
   client:send(":20," .. unit .. ",0" .. ",0" .. ",0")
end

function unsiege(unit)
   client:send(":21," .. unit .. ",0" .. ",0" .. ",0")
end

function cloak(unit)
   client:send(":22," .. unit .. ",0" .. ",0" .. ",0")
end

function decloak(unit)
   client:send(":23," .. unit .. ",0" .. ",0" .. ",0")
end

function loadUnit(unit, target)
   client:send(":26," .. unit .. "," .. target .. ",0" .. ",0")
end

function unloadUnit(unit, target)
   client:send(":27," .. unit .. "," .. target .. ",0" .. ",0")
end

function unloadAll(unit)
   client:send(":28," .. unit .. ",0" .. ",0" .. ",0")
end

function unloadAll(unit, x, y)
   client:send(":29,".. unit .. "," .. x .. "," .. y .. ",0")
end

function useTech(unit, techType)
   client:send(":38," .. unit .. "," .. techType .. ",0" .. ",0")
end

function useTech(unit, techType, x, y)
   client:send(":39," .. unit .. "," .. techType .. "," .. x .. "," .. y)
end

function useTech(unit, techType, target)
   client:send(":40," .. unit .. "," .. techType .. "," .. target .. ",0")
end

