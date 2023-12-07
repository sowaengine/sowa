#ifndef SW_LUA_SERVER_HXX
#define SW_LUA_SERVER_HXX

namespace sol {
class state;
} // namespace sol

class Node;
class LuaServer {
  public:
	static LuaServer &get();

	void Init();
	void Load();

	uint32_t NewBehaviour(const std::string &name);
	void DestroyBehaviour(uint32_t id);

	void StartBehaviour(Node *node, uint32_t id);
	void UpdateBehaviour(Node *node, uint32_t id);

  private:
	LuaServer();
	~LuaServer();

	sol::state *p_lua = nullptr;
	void *p_internal = nullptr;
};

#endif // SW_LUA_SERVER_HXX