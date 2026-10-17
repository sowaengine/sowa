local SPRITE_SIZE = 150
local ORBIT_RADIUS = 165
local SATELLITE_SCALE = 0.45
local OBJECT_SCENE = "res://scenes/object.scene"
local ALT_SCENE = "res://scenes/alt.scene"

world:component("Velocity2D", { velocity = Vector2(0, 0) })
world:component("MouseFollower", { speed = 900 })
world:component("Spin2D", { degreesPerSecond = 0 })

require("res://scripts/pulse.lua")
require("res://scripts/lifetime.lua")
require("res://scripts/ui_demo.lua")
require("res://scripts/parallax_demo.lua")
require("res://scripts/physics_queries_demo.lua")
require("res://scripts/fixed_gameplay_demo.lua")
require("res://scripts/character_controller.lua")

local Game = world:component("Game", {})

function Game:_start()
	self._prefab = resources:find_packed_scene(OBJECT_SCENE)
	local packed = resources:get(self._prefab)
	if packed == nil then
		log.warn("Packed scene {} is missing", OBJECT_SCENE)
	else
		log.info("Prefab {}", packed.path)
	end
	local scene = scenes:current()
	if scene == nil then
		log.error("Game _start: no current scene (main_scene is {})", settings.main_scene)
		return
	end
	log.info(
		"Game on {} - main_scene {} viewport {}",
		scene.name,
		settings.main_scene,
		Rect(0, 0, settings.viewport_width, settings.viewport_height)
	)
	self._camera = scene.active_camera
	for _, child in ipairs(scene:root():children()) do
		if child:has("Camera2D") then
			if self._camera == nil then
				self._camera = child
			end
		elseif child:has("Velocity2D") then
			local satellite = child:find("Satellite")
			if satellite ~= nil then
				local order = satellite:get("RenderOrder")
				if order ~= nil then
					order.layer = 1
					order.layerMode = "Explicit"
				end
				break
			end
		end
	end
	self._ticks = 0
	self._clock = timers:create(0.25, true, true)
	self._clock:on_timeout(function()
		self._ticks = (self._ticks or 0) + 1
	end)
	self._zoom_tween = nil
end

function Game:_exit()
	if self._clock ~= nil then
		self._clock:stop()
		self._clock:on_timeout(nil)
		self._clock = nil
	end
	if self._zoom_tween ~= nil then
		self._zoom_tween:stop()
		self._zoom_tween:on_update(nil)
		self._zoom_tween:on_finish(nil)
		self._zoom_tween = nil
	end
end

function Game:_update(dt)
	local scene = scenes:current()
	if scene == nil then
		return
	end

	if input:is_key_just_pressed(Key.N) then
		local next_scene = scene.name == "Alt" and settings.main_scene or ALT_SCENE
		log.info("Change queued to {} (current is still {})", next_scene, scene.name)
		scenes:change(next_scene)
	end

	local camera = self._camera
	if camera ~= nil and camera:is_alive() then
		local transform = camera:get("Transform2D")
		if transform ~= nil then
			local pan = 400 * dt
			if input:is_key_down(Key.Left) then
				transform.position.x = transform.position.x - pan
			end
			if input:is_key_down(Key.Right) then
				transform.position.x = transform.position.x + pan
			end
			if input:is_key_down(Key.Down) then
				transform.position.y = transform.position.y - pan
			end
			if input:is_key_down(Key.Up) then
				transform.position.y = transform.position.y + pan
			end

			local zoom = 1 + 0.9 * dt
			if input:is_key_down(Key.Q) then
				local s = math.clamp(transform.scale.x * zoom, 0.2, 8)
				transform.scale = Vector2(s, s)
			end
			if input:is_key_down(Key.E) then
				local s = math.clamp(transform.scale.x / zoom, 0.2, 8)
				transform.scale = Vector2(s, s)
			end
			if input:is_key_just_pressed(Key.T) then
				if self._zoom_tween ~= nil then
					self._zoom_tween:stop()
					self._zoom_tween:on_update(nil)
					self._zoom_tween:on_finish(nil)
					self._zoom_tween = nil
				end
				local camera_entity = camera
				local from = transform.scale.x
				local to = 2
				self._zoom_tween = tweens:create(0.4)
				self._zoom_tween:ease(Easing.QuadOut)
				self._zoom_tween:on_update(function(t)
					if not camera_entity:is_alive() then
						return
					end
					local xf = camera_entity:get("Transform2D")
					if xf == nil then
						return
					end
					local s = math.lerp(from, to, t)
					xf.scale = Vector2(s, s)
				end)
				self._zoom_tween:on_finish(function()
					self._zoom_tween = nil
				end)
			end
		end

		local camera2d = camera:get("Camera2D")
		if camera2d ~= nil then
			if input:is_key_just_pressed(Key.One) then
				camera2d.anchor = Vector2(0.5, 0.5)
			end
			if input:is_key_just_pressed(Key.Two) then
				camera2d.anchor = Vector2(0.25, 0.5)
			end
			if input:is_key_just_pressed(Key.Three) then
				camera2d.anchor = Vector2(0, 0)
			end
		end

		if input:is_key_just_pressed(Key.C) then
			if scene.active_camera ~= nil then
				scene.active_camera = nil
				log.info("Camera off")
			else
				scene.active_camera = camera
				log.info("Camera on {}", camera.name)
			end
		end
	end

	local objects = {}
	for _, child in ipairs(scene:root():children()) do
		if child:has("Velocity2D") then
			objects[#objects + 1] = child
		end
	end

	if input:is_key_just_pressed(Key.H) and objects[1] ~= nil and objects[1]:is_alive() then
		objects[1].visible = not objects[1].visible
		log.info("{} visible={}", objects[1].name, objects[1].visible)
	end

	if input:is_key_just_pressed(Key.Space) and #objects >= 2 then
		local follower_index = 1
		for i, object in ipairs(objects) do
			if object:has("MouseFollower") then
				follower_index = i
				break
			end
		end
		local next_index = follower_index % #objects + 1
		if objects[follower_index] ~= nil and objects[follower_index]:is_alive() then
			objects[follower_index]:remove("MouseFollower")
		end
		if objects[next_index] ~= nil and objects[next_index]:is_alive() then
			objects[next_index]:add("MouseFollower")
			log.info("Chaser moved from {} to {}", objects[follower_index].name, objects[next_index].name)
		end
	end

	if input:is_key_just_pressed(Key.B) then
		local n = 0
		for _, object in ipairs(objects) do
			if object:has("Pulse") then
				object:get("Pulse"):boost()
				n = n + 1
			end
		end
		log.info("Pulse boost on {} objects, dt {}", n, dt)
	end

	if not input:is_key_just_pressed(Key.F) then
		return
	end
	if self._prefab == nil then
		log.warn("Cannot spawn: packed scene {} is missing", OBJECT_SCENE)
		return
	end
	local follower = nil
	for _, object in ipairs(objects) do
		if object:has("MouseFollower") then
			follower = object
			break
		end
	end
	if follower == nil or not follower:is_alive() then
		log.warn("Cannot spawn: no mouse follower")
		return
	end
	local spawned = resources:instantiate(self._prefab, follower)
	if spawned == nil then
		log.error("Failed to instantiate {}", OBJECT_SCENE)
		return
	end
	local angle = math.random() * math.pi * 2
	local transform = spawned:get("Transform2D")
	if transform ~= nil then
		transform.position = Vector2(math.cos(angle) * ORBIT_RADIUS, math.sin(angle) * ORBIT_RADIUS)
		transform.scale = Vector2(SATELLITE_SCALE, SATELLITE_SCALE)
	end
	local sprite = spawned:get("Sprite")
	if sprite ~= nil then
		sprite.modulate = Color.hsv(angle / math.pi * 180, 0.6, 1)
	end
	spawned:remove("Velocity2D")
	spawned:remove("Spin2D")
	spawned:add("Lifetime")
	local order = spawned:get("RenderOrder")
	if order ~= nil then
		order.zIndex = -1
	end
	local nested = spawned:find("Satellite")
	if nested ~= nil then
		nested:destroy()
	end
	log.info("Spawned satellite on {} at angle {}", follower.name, angle)
end

world:system("Bounce")
	:with("Transform2D", "Velocity2D")
	:without("MouseFollower")
	:each(function(e, transform, velocity)
		local dt = world.delta_time
		local half = SPRITE_SIZE * 0.5
		local bounds = Rect(half, half, settings.viewport_width - SPRITE_SIZE,
			settings.viewport_height - SPRITE_SIZE)
		local pos = transform.position
		local vel = velocity.velocity
		local x = pos.x + vel.x * dt
		local y = pos.y + vel.y * dt
		local vx = vel.x
		local vy = vel.y
		-- Set the inward sign. A toggle (vx = -vx) flips again when a
		-- tiny dt rounds the step to 0 at the wall (window move) and the
		-- entity glides along the edge.
		if x < bounds:left() then
			x = bounds:left()
			vx = math.abs(vx)
		elseif x > bounds:right() then
			x = bounds:right()
			vx = -math.abs(vx)
		end
		if y < bounds:bottom() then
			y = bounds:bottom()
			vy = math.abs(vy)
		elseif y > bounds:top() then
			y = bounds:top()
			vy = -math.abs(vy)
		end
		transform.position = Vector2(x, y)
		velocity.velocity = Vector2(vx, vy)
	end)

world:system("Spin")
	:with("Transform2D", "Spin2D")
	:each(function(e, transform, spin2d)
		transform.rotation = transform.rotation + spin2d.degreesPerSecond * world.delta_time
	end)

world:system("Follow")
	:with("Transform2D", "MouseFollower")
	:each(function(e, transform, follower)
		local dt = world.delta_time
		local viewport = Rect(0, 0, settings.viewport_width, settings.viewport_height)
		local target = input:mouse_position()
		if not viewport:contains(target) then
			return
		end
		local dx = target.x - transform.position.x
		local dy = target.y - transform.position.y
		local distance = math.sqrt(dx * dx + dy * dy)
		local step = follower.speed * dt
		if distance <= step or distance <= 0.0001 then
			transform.position = target
		else
			local t = math.clamp(step / distance, 0, 1)
			transform.position = math.lerp(transform.position, target, t)
		end
	end)

-- Always: M plays the clip on the mouse chaser. spatial uses that
-- entity's Transform2D versus the camera, so pan follows the object.
world:system("PreviewSound")
	:with("AudioSource", "MouseFollower")
	:each(function(e, source)
		if input:is_key_just_pressed(Key.M) then
			source:play()
		end
	end)

local Crate = world:component("Crate", {})

function Crate:_collision_begin(other)
	if self._hit then
		return
	end
	self._hit = true
	log.info("Crate {} hit {}", self.entity.name, other.name)
end
