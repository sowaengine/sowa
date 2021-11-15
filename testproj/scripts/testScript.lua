-- testScript.lua
print("script begin")
function init()
	TestScript = {}
   print("init")



	TestScript.Export = {
		exp_variable = 10
	}

	local this = get_node("1")
	local transform = this.Transform()
	local name = this.name

	function TestScript:Start()
		print("script start")
	end

	local x = 60
	function TestScript:Update()
		x = x + 1
		if x >= 360 then
			print("script update: " .. x)
			x = 0
		end
		
		--get(this, 0)
		
		--target API
		-- transform.translation -> vector3 table
		-- transform:translate(vec3)
		-- transform:translate_x(vec3)
		-- transform:translate_y(vec3)
		-- transform:translate_z(vec3)
		-- transform:look_at(vec3 pos, vec3 up{0, 1, 0})
		
		
		--local aaa = transform.translation
		this.name = tostring(x)
		local rad = math.rad(x)

		transform.translation = Vector3(math.sin(rad)*100, math.cos(rad)*100, 0)
		transform.rotation = Vector3(0, 0, rad)

		
		
		--transform.translation = 10

	end




	return TestScript
end