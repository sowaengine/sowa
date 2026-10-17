-- Moves its entity with the project's move_* input actions (WASD by
-- default; rebind them in Project Settings > Input).

local CharacterController = world:component("CharacterController", {
	speed = 600,
})

function CharacterController:_update(dt)
	local direction = input:get_vector("move_left", "move_right", "move_down", "move_up")
	-- Diagonals would otherwise be faster than straight movement.
	direction:normalize()

	local transform = self.entity:get("Transform2D")
	transform.position = transform.position + direction * self.speed * dt
end
