#ifndef _E_NODE_HPP__
#define _E_NODE_HPP__

#include "sowa.hpp"
#include "stlpch.hpp"
#include "object_type.hpp"
#include "core/file_buffer.hpp"

namespace Sowa {
class Scene;
class Application;

enum class PauseMode {
	Inherit = 0,
	Ignore,
	Stop,
};

class Node : public ObjectType {
  public:
	Node();
	Node(const std::string &name);
	virtual ~Node();

	static std::string Typename() { return "Sowa::Node"; }

	inline std::string &Name() { return _Name; }
	inline bool IsValid() { return _pScene.lock() != nullptr; }

	// Called when entity joins / exits scene tree
	virtual void EnterScene() {}
	virtual void ExitScene() {}

	virtual void UpdateLogic() {}
	virtual void UpdateDraw() {}

	inline PauseMode GetPauseMode() { return _PauseMode; }
	void SetPauseMode(PauseMode mode) { _PauseMode = mode; }

	void Lock() { _Locked = true; }
	void Unlock() { _Locked = false; }
	bool IsLocked() { return _Locked; }

	inline Node *GetParent() { return _Parent; }
	inline std::vector<Node *> GetChildren() { return _Children; }

	void AddChild(Node *node);
	Node *GetChild(const std::string &name);
	void RemoveChild(const std::string &name);
	void RemoveNode(Node *node);
	void SetParent(Node *node);

	const std::string& GetNodeType() { return _NodeType; }

	static FileBuffer SaveImpl(ObjectType* out);
	static bool LoadImpl(ObjectType* out, const FileBuffer& buf);

  private:
	friend class Scene;
	friend class Application;
	std::string _Name;

	bool _Locked{false};

	std::weak_ptr<Scene> _pScene{};
	std::vector<Node *> _Children;
	Node *_Parent{nullptr};

	PauseMode _PauseMode{PauseMode::Inherit};

  protected:
	std::string _NodeType{"Node"};
};
} // namespace Sowa

#endif // _E_NODE_HPP__