-- Lifetime: F-spawned satellites expire. _exit counts them on Game.

local Lifetime = world:component("Lifetime", { seconds = 6 })

function Lifetime:_update(dt)
	self.seconds = self.seconds - dt
	if self.seconds <= 0 then
		self.entity:queue_destroy()
	end
end

function Lifetime:_exit()
	local scene = scenes:current()
	if scene == nil then
		return
	end
	local game = scene:root():get("Game")
	if game ~= nil then
		game._expired = (game._expired or 0) + 1
		log.info("Satellite expired - count {}", game._expired)
	end
end

return Lifetime
