#ifndef SW_GAME_HXX
#define SW_GAME_HXX
#pragma once

#include "scene/scene.hxx"

void Main();

class TankMovement {
  public:
	static void Start(Node *, Behaviour *);
	static void Update(Node *, Behaviour *);
};

class MouseShooter {
  public:
	static void Start(Node *, Behaviour *);
	static void Update(Node *, Behaviour *);
};

#endif // SW_GAME_HXX