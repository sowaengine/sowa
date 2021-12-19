#include "Keyboard.hpp"

#include <GLFW/glfw3.h>
#include <unordered_map>

#include "Input.hpp"


namespace Ease
{

   std::unordered_map<KeyCode, KeyState> keyStates;


   void Input::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
   {
      keyStates[KeyCode(key)] = KeyState(action);
   }

   bool Input::IsKeyPressed(KeyCode key)
   {
      return keyStates[key] == KeyState::PRESS || keyStates[key] == KeyState::REPEAT;
   }
   
} // namespace Ease
