#ifndef SW_TOP_DOWN_EIGHT_DIR_HXX
#define SW_TOP_DOWN_EIGHT_DIR_HXX
#pragma once

#include "scene/node.hxx"

class Behaviour;

class TopDownEightDirMovement {
  public:
	static void Start(Node *, Behaviour *);
	static void Update(Node *, Behaviour *);
};

#endif // SW_TOP_DOWN_EIGHT_DIR_HXX