#ifndef SW_BEHAVIOUR_HXX
#define SW_BEHAVIOUR_HXX
#pragma once

#include <any>
#include <functional>
#include <string>
#include <unordered_map>

#include "core/error.hxx"
#include "servers/script_server.hxx"

class Node;
class Behaviour;

using BehaviourFunc = std::function<void(Node *, Behaviour *)>;

class Behaviour {
  public:
	static Behaviour New(BehaviourFunc startFunc, BehaviourFunc updateFunc);

	void Start(Node *);
	void Update(Node *);
	ErrorCode CallFunc(Node *node, const std::string &decl, ScriptFunctionCaller &caller);

	inline void SetStartFunc(BehaviourFunc func) { m_startFunc = func; }
	inline void SetUpdateFunc(BehaviourFunc func) { m_updateFunc = func; }

	inline std::unordered_map<std::string, std::any> &DataTable() { return m_dataTable; }

	std::string GetBehaviourName();
	size_t GetBehaviourID();

  private:
	friend class BehaviourDB;

	BehaviourFunc m_startFunc;
	BehaviourFunc m_updateFunc;

	std::unordered_map<std::string, std::any> m_dataTable;

	size_t m_behaviourID = 0;
};

#endif // SW_BEHAVIOUR_HXX