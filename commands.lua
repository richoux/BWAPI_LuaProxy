-- commands
function attack(unit, x, y)
   sock:send(":1," .. unit .. "," .. x .. "," .. y .. ",0")
end

function attack(unit, target)
   sock:send(":2," .. unit .. "," .. target .. ",0" .. ",0")
end

function rightClick(unit, x, y)
   sock:send(":3,".. unit .. "," .. x .. "," .. y .. ",0")
end

function rightClick(unit, target)
   sock:send(":4," .. unit .. "," .. target .. ",0" .. ",0")
end

function stop(unit)
   sock:send(":10," .. unit .. ",0" .. ",0" .. ",0")
end

function holdPosition(unit)
   sock:send(":11," .. unit .. ",0" .. ",0" .. ",0")
end

function patrol(unit, x, y)
   sock:send(":12,".. unit .. "," .. x .. "," .. y .. ",0")
end

function follow(unit, target)
   sock:send(":13," .. unit .. "," .. target .. ",0" .. ",0")
end

function repair(unit, target)
   sock:send(":16," .. unit .. "," .. target .. ",0" .. ",0")
end

function burrow(unit)
   sock:send(":18," .. unit .. ",0" .. ",0" .. ",0")
end

function unburrow(unit)
   sock:send(":19," .. unit .. ",0" .. ",0" .. ",0")
end

function siege(unit)
   sock:send(":20," .. unit .. ",0" .. ",0" .. ",0")
end

function unsiege(unit)
   sock:send(":21," .. unit .. ",0" .. ",0" .. ",0")
end

function cloak(unit)
   sock:send(":22," .. unit .. ",0" .. ",0" .. ",0")
end

function decloak(unit)
   sock:send(":23," .. unit .. ",0" .. ",0" .. ",0")
end

function loadUnit(unit, target)
   sock:send(":26," .. unit .. "," .. target .. ",0" .. ",0")
end

function unloadUnit(unit, target)
   sock:send(":27," .. unit .. "," .. target .. ",0" .. ",0")
end

function unloadAll(unit)
   sock:send(":28," .. unit .. ",0" .. ",0" .. ",0")
end

function unloadAll(unit, x, y)
   sock:send(":29,".. unit .. "," .. x .. "," .. y .. ",0")
end

function useTech(unit, techType)
   sock:send(":38," .. unit .. "," .. techType .. ",0" .. ",0")
end

function useTech(unit, techType, x, y)
   sock:send(":39," .. unit .. "," .. techType .. "," .. x .. "," .. y)
end

function useTech(unit, techType, target)
   sock:send(":40," .. unit .. "," .. techType .. "," .. target .. ",0")
end
