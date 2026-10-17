local Demo = world:component("UIDemoController", {})

function Demo:_start()
  local canvas = self.entity:find("Canvas")
  local content = canvas:find("Content")
  local settings = content:find("Settings")
  local status = settings:find("Status"):get("UILabel")
  local modal = canvas:find("Modal")
  ui.on(settings:find("Actions/Save"), "clicked", function()
    local name = settings:find("Name"):get("UITextInput").text
    status.text = "Saved settings for " .. (name ~= "" and name or "Player") .. "."
  end)
  ui.on(settings:find("Actions/OpenModal"), "clicked", function() ui.open(modal) end)
  ui.on(modal:find("Close"), "clicked", function() ui.close(modal) end)
  ui.on(settings:find("Volume"), "changed", function(event)
    settings:find("Progress"):get("UIProgressBar").value = event.value
  end)
  local dark = canvas:get("UICanvas").theme
  local light = 990103
  local useLight = false
  ui.on(canvas:find("Theme"), "clicked", function()
    useLight = not useLight
    canvas:get("UICanvas").theme = useLight and light or dark
  end)
  ui.on(content:find("Choices/List"), "changed", function(event)
    status.text = "Selected: " .. event.text
  end)
end
