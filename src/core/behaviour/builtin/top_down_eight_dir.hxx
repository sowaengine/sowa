#ifndef SW_TOP_DOWN_EIGHT_DIR_HXX
#define SW_TOP_DOWN_EIGHT_DIR_HXX
#pragma once

#include "scene/node.hxx"

class TopDownEightDirMovement {
  public:
	static void Start(Node *);
	static void Update(Node *);
};

#endif // SW_TOP_DOWN_EIGHT_DIR_HXX