#ifndef _E_SCRIPTSERVER_HPP__
#define _E_SCRIPTSERVER_HPP__
#pragma once

namespace Sowa {
class ScriptServer {
  public:
	ScriptServer(const ScriptServer &) = delete;
	ScriptServer(const ScriptServer &&) = delete;
	ScriptServer &operator=(const ScriptServer &) = delete;
	ScriptServer &operator=(const ScriptServer &&) = delete;

  protected:
	ScriptServer();
	virtual ~ScriptServer();
};
} // namespace Sowa

#endif // _E_SCRIPTSERVER_HPP__