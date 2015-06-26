local socket = require('socket')
local server = assert(socket.bind('*', 13337))
local mp = require 'MessagePack'
local ip, port = server:getsockname()
print(ip)
print(port)

-- local selfId
-- local players = {}
-- local playerRace = {}
-- local ally = {}
-- local mapName
-- local mapWidth
-- local mapHeight

   print("avant accept")
   local client = server:accept()
   print("apres accept")

while true do

   --client:settimeout(10)
   --local line, err = client:receive()
   --if not err then client:sent(line .. "\n") end
   
   --local s, status, partial = client:receive()
   print("Before receive");
   print(client);
   local mpac = client:receive()
   print("After receive");

   print("Eval " .. mpac);
   --loadstring(mpac)();
   
   -- print("Self id: " .. selfId);
   -- for p in players do
   --    print("Player id: " .. p);
   --    print("Player race: " .. playerRace[p]);
   --    print("Player " .. p .. " is an ally: " .. ally[p]);
   -- end

   -- print("Map name: " .. mapName);
   -- print("Map Width: " .. mapWidth);
   -- print("Map Height: " .. mapHeight);

   -- local data = {};
   -- for i, v in mp.unpacker(mpac) do
   --    data[i] = v;
   --    --print("i=" .. i .. ", v=" .. v);
   -- end

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
   
   --print(s or partial)
   if status == "closed" then break end
   client:send("0\n")
end

client:close()
