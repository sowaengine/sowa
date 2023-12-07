#include "behaviour.hxx"
#include "behaviour_db.hxx"

#include "servers/script_server.hxx"
#include "utils/utils.hxx"

#include "angelscript.h"

Behaviour Behaviour::New(std::string extends, BehaviourFunc startFunc, BehaviourFunc updateFunc) {
	Behaviour behaviour;
	behaviour.extends = extends;
	behaviour.m_startFunc = startFunc;
	behaviour.m_updateFunc = updateFunc;

	return behaviour;
}

void Behaviour::Start(Node *node) {
	if (m_startFunc)
		m_startFunc(node, this);
}
void Behaviour::Update(Node *node) {
	if (m_updateFunc)
		m_updateFunc(node, this);
}

ErrorCode Behaviour::CallFunc(Node *node, const std::string &decl, ScriptFunctionCaller &caller) {
	if (DataTable().find("m_obj") == DataTable().end()) {
		Utils::Error("Behaviour::CallFunc can only be used in script behaviours");
		return ERR_FAILED;
	}

	std::any &any = DataTable()["m_obj"];
	if (!any.has_value()) {
		Utils::Error("Behaviour::CallFunc can only be used in script behaviours");
		return ERR_FAILED;
	}

	asIScriptObject **o = std::any_cast<asIScriptObject *>(&any);
	if (!o) {
		return ERR_FAILED;
	}

	asIScriptObject *obj = *o;

	caller = ScriptServer::get().begin_method(GetBehaviourName(), decl);
	caller.arg_this((void *)obj);
	return OK;
}

std::string Behaviour::GetBehaviourName() {
	if (m_behaviourID == 0)
		return "";

	return BehaviourDB::get().GetBehaviourName(m_behaviourID);
}

size_t Behaviour::GetBehaviourID() {
	return m_behaviourID;
}