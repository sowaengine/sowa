#ifndef _E_NATIVEBEHAVIOURCLASS_HPP__
#define _E_NATIVEBEHAVIOURCLASS_HPP__

#pragma once

#include "Ease.hpp"
#include <string>

namespace Ease::Component {
class NativeBehaviourClass {
  public:
	NativeBehaviourClass();
	~NativeBehaviourClass();

	std::string &ClassName() { return m_ClassName; }
	Ease::NativeBehaviour *&Behaviour() { return pBehaviour; }
	Ease::NativeBehaviourFactory *&Factory() { return pFactory; }

  private:
	std::string m_ClassName = "";

	Ease::NativeBehaviourFactory *pFactory = nullptr;
	Ease::NativeBehaviour *pBehaviour = nullptr;
};
} // namespace Ease::Component

#endif