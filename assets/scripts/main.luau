-- DjusovEngine Starter Script
local World = require("@de/world")
local Players = require("@de/players")
local SaveManager = require("@de/savemanager")
local Lighting = require("@de/lighting")

local timeElapsed = 0.0

function onInit()
    print("[Luau] Script initialized successfully!")
end

function onUpdate(dt: number)
    timeElapsed = timeElapsed + dt

    -- Dynamic subtle pulse of street lights
    local lamp = World.getObject("Street_Lamp")
    if lamp then
        local pulse = 10.0 + math.sin(timeElapsed * 2.0) * 2.0
    end
end

function onHit(entityId: number, damage: number, attacker: string)
    print(string.format("[Luau] Entity %d took %.1f damage from %s", entityId, damage, attacker))
end

function onTouch(entityId: number, playerName: string)
    print(string.format("[Luau] Entity %d touched by %s", entityId, playerName))
end
