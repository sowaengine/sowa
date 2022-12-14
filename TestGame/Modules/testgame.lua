local testgame = {}
testgame.author = "Sowa"
testgame.name = "Test Game"
testgame.version = 1

local playercontroller = LuaBehaviour.new("Player Controller", "testgame.playercontroller")
playercontroller.start = function(entity)

end

playercontroller.update = function(entity)
    local tc = entity:get_component(Component.Transform2D)
    local input = Vector2.new(0, 0)
    if Input.is_key_down(Key.KEY_W) then
        input.y = input.y + 1
    end
    if Input.is_key_down(Key.KEY_S) then
        input.y = input.y - 1
    end

    if Input.is_key_down(Key.KEY_D) then
        input.x = input.x + 1
    end
    if Input.is_key_down(Key.KEY_A) then
        input.x = input.x - 1
    end
    if input:length() > 0.1 then
        input = input:clamp(1.0)
    end

    tc.position.x = tc.position.x + (input.x * 8)
    tc.position.y = tc.position.y + (input.y * 8)
end

playercontroller:register()



testgame.start = function()

end

testgame.update = function()

end

testgame.gui_update = function()

end

return testgame
