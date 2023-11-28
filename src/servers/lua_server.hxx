#ifndef SW_LUA_SERVER_HXX
#define SW_LUA_SERVER_HXX

class LuaServer {
  public:
	static LuaServer &get();

	void Init();

  private:
	LuaServer();
	~LuaServer();
};

#endif // SW_LUA_SERVER_HXX