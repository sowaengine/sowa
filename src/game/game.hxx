#ifndef SW_GAME_HXX
#define SW_GAME_HXX
#pragma once

#include "scene/scene.hxx"

void Main();

class TankMovement {
  public:
	static void Start(Node *);
	static void Update(Node *);
};

class TankBarrelBehaviour {
  public:
	static void Start(Node *);
	static void Update(Node *);
};

class BulletMovement {
  public:
	static void Start(Node *);
	static void Update(Node *);
};

#endif // SW_GAME_HXX