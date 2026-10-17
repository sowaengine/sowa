-- Open physics_queries.scene and use Play Current (F6).
local Demo = world:component("PhysicsQueriesDemo", {})

function Demo:_start()
	self._probe = self.entity:find("Probe")
	self._ray = self._probe:get("Raycast2D")
	self._box = self.entity:find("Box")
	self._excluded = false
	self._maskName = "all layers"
end

local function describe(hit)
	if hit == nil then
		return "miss"
	end
	return string.format("%s / %.0f px%s", hit.collider.name, hit.distance,
		hit.isSensor and " / sensor" or "")
end

local function marker(entity, hit, offset)
	entity.visible = hit ~= nil
	if hit ~= nil then
		entity:get("Transform2D").position = hit.point + Vector2(0, offset)
	end
end

function Demo:_update(dt)
	local transform = self._probe:get("Transform2D")
	local ray = self._ray
	local movement = Vector2(
		(input:is_key_down(Key.Right) and 1 or 0) - (input:is_key_down(Key.Left) and 1 or 0),
		(input:is_key_down(Key.Up) and 1 or 0) - (input:is_key_down(Key.Down) and 1 or 0))
	transform.position = transform.position + movement:normalized() * 260 * dt
	transform.rotation = transform.rotation + 45 * dt *
		((input:is_key_down(Key.Q) and 1 or 0) - (input:is_key_down(Key.E) and 1 or 0))

	if input:is_key_just_pressed(Key.One) then
		ray.collisionMask = physics.ALL_LAYERS
		self._maskName = "all layers"
	elseif input:is_key_just_pressed(Key.Two) then
		ray.collisionMask = physics.layers(2)
		self._maskName = "layer 2"
	elseif input:is_key_just_pressed(Key.Three) then
		ray.collisionMask = physics.layers(3)
		self._maskName = "layer 3"
	end
	if input:is_key_just_pressed(Key.S) then
		ray.includeSensors = not ray.includeSensors
	end
	if input:is_key_just_pressed(Key.X) then
		self._excluded = not self._excluded
		if self._excluded then
			ray:add_body_exception(self._box)
		else
			ray:remove_body_exception(self._box)
		end
	end
	if input:is_key_just_pressed(Key.Space) then
		ray.enabled = not ray.enabled
	end
	if input:is_key_just_pressed(Key.R) then
		transform.position = Vector2(200, 600)
		transform.rotation = 0
		ray.enabled = true
		ray.includeSensors = false
		ray.collisionMask = physics.ALL_LAYERS
		ray:clear_exceptions()
		self._excluded = false
		self._maskName = "all layers"
	end
	if input:is_key_just_pressed(Key.F) then
		ray:force_update()
	end

	-- Direct queries use world endpoints. This probe has no parent transform,
	-- so rotating its local endpoint is sufficient to match Raycast2D.
	local angle = transform.rotation * math.pi / 180
	local localTarget = ray.targetPosition
	local delta = Vector2(localTarget.x * math.cos(angle) - localTarget.y * math.sin(angle),
		localTarget.x * math.sin(angle) + localTarget.y * math.cos(angle))
	local origin = transform.position
	local target = origin + delta
	local exclusions = { self._probe } -- Matches the component's excludeOwner.
	if self._excluded then
		table.insert(exclusions, self._box)
	end
	local options = {
		collisionLayer = ray.collisionLayer,
		collisionMask = ray.collisionMask,
		includeSensors = ray.includeSensors,
		excludeBodies = exclusions,
	}
	local direct = physics.raycast(origin, target, options)
	local hits = physics.raycast_all(origin, target, options)
	local cached = ray:hit()

	local line = self.entity:find("RayLine")
	local lineTransform = line:get("Transform2D")
	lineTransform.position = origin + delta * 0.5
	lineTransform.rotation = transform.rotation
	line:get("Sprite").size = Vector2(delta:length(), 3)
	marker(self.entity:find("DirectHit"), direct, 14)
	marker(self.entity:find("CachedHit"), cached, -14)

	local hitNames = {}
	for _, hit in ipairs(hits) do
		table.insert(hitNames, hit.collider.name)
	end
	self.entity:find("Status"):get("Text2D").text = string.format(
		"Mask: %s   |   Sensors: %s   |   Box excluded: %s   |   Auto update: %s\n" ..
		"Direct (orange): %s\nComponent cache (green): %s\nAll hits, nearest first: %s",
		self._maskName, ray.includeSensors and "on" or "off", self._excluded and "yes" or "no",
		ray.enabled and "on" or "off", describe(direct), describe(cached),
		#hitNames > 0 and table.concat(hitNames, " -> ") or "none")
end
