
function _init()
	Player = {}

   local speed = 10
   local transform

   local MoveKeys = {
      UP = Key.W,
      DOWN = Key.S,
      LEFT = Key.A,
      RIGHT = Key.D,
   }

   function Player:Start()
      transform = this:GetTransform2D()
      
      if this.name == "Player 2" then
         MoveKeys.RIGHT = Key.RIGHT
         MoveKeys.LEFT  = Key.LEFT
         MoveKeys.UP    = Key.UP
         MoveKeys.DOWN  = Key.DOWN
      end

   end

   function Player:Update()
      local axis = Vector2.new()
      
      if Input.IsKeyPressed(MoveKeys.LEFT) then
         axis.x = axis.x - 1
      end
      if Input.IsKeyPressed(MoveKeys.RIGHT) then
         axis.x = axis.x + 1
      end

      if Input.IsKeyPressed(MoveKeys.UP) then
         axis.y = axis.y - 1
      end
      if Input.IsKeyPressed(MoveKeys.DOWN) then
         axis.y = axis.y + 1
      end

      transform.position.x = transform.position.x + (axis.x * speed)
      transform.position.y = transform.position.y + (axis.y * speed)
   end


	return Player
end