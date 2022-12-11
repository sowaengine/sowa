local testgame = {}
testgame.author = "Sowa"
testgame.name = "Test Game"
testgame.version = 1

local playercontroller = LuaBehaviour.new("Player Controller", "testgame.playercontroller")
playercontroller.start = function(entity)

end

playercontroller.update = function(entity)
    local tc = entity:get_component(Component.Transform2D)
    tc.position.x = tc.position.x + 1
end

playercontroller:register()



testgame.start = function()

end

testgame.update = function()

end

testgame.gui_update = function()

end

return testgame
