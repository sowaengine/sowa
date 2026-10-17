-- Loaded by example/project.sowa. Open Ctrl+Shift+P and search "Example".
local input = editor.input

editor.commands.register {
	id = "example.hello",
	title = "Example: Hello World",
	allow_while_playing = true,
	run = function()
		log.info("Hello from an editor command!")
	end,
}

editor.commands.register {
	id = "example.send_message",
	title = "Example: Send Message to the Server",
	keywords = "input text select workflow",
	allow_while_playing = true,
	run = function()
		local message = input.text {
			title = "What message should be sent?",
			placeholder = "Type your message and press Enter",
			required = true,
			validate = function(value)
				if #value > 200 then
					return "Keep the message under 200 characters."
				end
			end,
		}

		local server = input.select {
			title = "Which server to send to?",
			placeholder = "Choose a server",
			options = { "Server A", "Server B", "Server C" },
		}

		-- Demo transport: replace this with your own server integration.
		log.info("[Demo] Message to {}: {}", server, message)
	end,
}

editor.commands.register {
	id = "example.project_note",
	title = "Example: Write a Project Note",
	keywords = "default value input",
	run = function()
		local category = input.select {
			title = "Choose a note category",
			options = {
				{ label = "Gameplay", value = "gameplay" },
				{ label = "Art and Animation", value = "art" },
				{ label = "Audio", value = "audio" },
			},
		}
		local note = input.text {
			title = "What needs attention?",
			value = "Review the " .. category .. " before release",
			required = true,
		}
		log.info("[Note / {}] {}", category, note)
	end,
}
