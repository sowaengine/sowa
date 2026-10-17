local TopDownPlayer = world:component("TopDownPlayer", {
	speed = 400,
})

function TopDownPlayer:_update(dt)
	local x, y = 0, 0
	if input:is_key_down(Key.A) or input:is_key_down(Key.Left) then
		x = x - 1
	end
	if input:is_key_down(Key.D) or input:is_key_down(Key.Right) then
		x = x + 1
	end
	if input:is_key_down(Key.S) or input:is_key_down(Key.Down) then
		y = y - 1
	end
	if input:is_key_down(Key.W) or input:is_key_down(Key.Up) then
		y = y + 1
	end

	local direction = Vector2(x, y):normalized()
	local transform = self.entity:get("Transform2D")
	transform.position = transform.position + direction * self.speed * dt

	local camera = self.entity:find("Camera"):get("Transform2D")
	local zoom = camera.scale.x
	if input:is_key_down(Key.Q) then
		zoom = zoom + dt
	end
	if input:is_key_down(Key.E) then
		zoom = zoom - dt
	end
	zoom = math.clamp(zoom, 0.4, 2)
	camera.scale = Vector2(zoom, zoom)

	if input:is_key_just_pressed(Key.R) then
		transform.position = Vector2(0, 0)
		camera.scale = Vector2(1, 1)
	end
end
