-- Pulse: many entities, one type. Fields in ECS; _t / _baseX / _baseY are
-- per-entity extras.

local Pulse = world:component("Pulse", { amplitude = 0.08, speed = 4 })

function Pulse:_start()
	local transform = self.entity:get("Transform2D")
	if transform == nil then
		return
	end
	self._baseX = transform.scale.x
	self._baseY = transform.scale.y
	self._t = 0
end

function Pulse:_update(dt)
	if self._baseX == nil then
		return
	end
	self._t = self._t + dt * self.speed
	local transform = self.entity:get("Transform2D")
	if transform == nil then
		return
	end
	local s = math.map(math.sin(self._t), -1, 1, 1 - self.amplitude, 1 + self.amplitude)
	transform.scale = Vector2(self._baseX * s, self._baseY * s)
end

function Pulse:_exit()
	local transform = self.entity:get("Transform2D")
	if transform == nil or self._baseX == nil then
		return
	end
	transform.scale = Vector2(self._baseX, self._baseY)
end

function Pulse:boost()
	self.amplitude = self.amplitude + 0.05
end

return Pulse
