-- Open fixed_gameplay.scene and use Play Current (F6).
-- Sample frame input here, then consume physics commands only on fixed ticks.
local Demo = world:component("FixedGameplayDemo", { frames = 0, ticks = 0, kicks = 0 })

function Demo:_start()
	self._ship = self.entity:find("Ship")
	self._status = self.entity:find("Status")
	self._thrust = Vector2(0, 0)
	self._kick = false
end

function Demo:_show()
	self._status:get("Text2D").text = string.format(
		"Frame updates: %d    Physics ticks: %d    Buffered impulses: %d\nF7 pauses; each F9 advances one frame update and one physics tick.",
		self.frames, self.ticks, self.kicks)
end

function Demo:_update(dt)
	self.frames = self.frames + 1
	self._thrust = Vector2(
		(input:is_key_down(Key.D) and 1 or 0) - (input:is_key_down(Key.A) and 1 or 0),
		(input:is_key_down(Key.W) and 1 or 0) - (input:is_key_down(Key.S) and 1 or 0)):normalized()
	-- Retain a discrete action until a tick consumes it, even on a zero-tick frame.
	self._kick = self._kick or input:is_key_just_pressed(Key.Space)
	self:_show()
end

function Demo:_physics_update(dt)
	local body = self._ship:get("RigidBody2D")
	-- A force submission lasts one physics tick. Resubmit continuous thrust each tick.
	body:apply_force(self._thrust * 1200)
	if self._kick then
		body:apply_impulse(Vector2(0, 150))
		self._kick = false
		self.kicks = self.kicks + 1
	end
	self.ticks = self.ticks + 1
	self:_show()
end
