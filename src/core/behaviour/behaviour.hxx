#ifndef SW_BEHAVIOUR_HXX
#define SW_BEHAVIOUR_HXX
#pragma once

#include <functional>
#include <string>

class Node;

using BehaviourFunc = std::function<void(Node *)>;

class Behaviour {
  public:
	static Behaviour New(BehaviourFunc startFunc, BehaviourFunc updateFunc);

	void Start(Node *);
	void Update(Node *);

	std::string GetBehaviourName();
	size_t GetBehaviourID();

  private:
	friend class BehaviourDB;

	BehaviourFunc m_startFunc;
	BehaviourFunc m_updateFunc;

	// todo: Behaviour data table
	// unordered_map<string, any>

	size_t m_behaviourID = 0;
};

#endif // SW_BEHAVIOUR_HXX