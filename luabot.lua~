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

   local mpac = client:receive()

   print("Eval " .. mpac);
   loadstring(mpac)();
   
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

   -- io.write("Minerals: " );
   -- print(data[1]);
   
   -- io.write("Gas: " );
   -- print(data[2]);
   
   -- io.write("Supply: " );
   -- if( data[4] ~= nil ) then
   --    print(data[3] .. "/" .. data[4]);
   -- else
   --    print(data[3]);
   -- end
   
   if status == "closed" then break end
   client:send("0\n")
end

client:close()
