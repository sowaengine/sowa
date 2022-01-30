
function _init()
	Camera = {}

	
	speed = 10
	
	

	local player1 = GetEntityByName("Player 1")
	local player2 = GetEntityByName("Player 2")
	
	local p1transform
	local p2transform

	local transform

	function Camera:Start()
		transform = this:GetTransform2D()


		p1transform = player1:GetTransform2D()
		p2transform = player2:GetTransform2D()
	end


	function Camera:Update()
		local midPoint = Vector2.new()

		midPoint.x = (p1transform.position.x + p2transform.position.x) / 2
		midPoint.y = (p1transform.position.y + p2transform.position.y) / 2

		transform.position = midPoint
	end




	return Camera
end