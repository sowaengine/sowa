#ifndef SW_INPUT_SERVER_HXX
#define SW_INPUT_SERVER_HXX
#pragma once

class InputServer {
  public:
	InputServer();
	static InputServer &GetInstance();

	void ProcessInput();
	void PollEvents();

	void GetMousePosition(double &x, double &y);

	bool IsKeyDown(int key);
};

#endif // SW_INPUT_SERVER_HXX