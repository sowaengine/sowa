# Sowa Engine for VS Code

Debug Lua scripts in a running Sowa editor or game: breakpoints (with
conditions, hit counts and logpoints), stepping, call stack, variables, Watch,
Debug Console and "Uncaught Errors".

The engine is the debug adapter. Editor builds and debug export templates
listen on `127.0.0.1:6510`; this extension connects VS Code to that port.

## Build and install

```bash
npm install
npm run package
code --install-extension sowa.vsix
```

## Use

1. Open the project directory (the one holding `project.sowa`) in VS Code.
2. Start the editor or game. To debug startup code, start it with
   `--wait-for-debugger`; the editor then waits on the first Play.
3. Press F5. Without a `launch.json` the extension attaches to port 6510.

`launch.json`:

```json
{
  "type": "sowa",
  "request": "attach",
  "name": "Attach to Sowa",
  "port": 6510
}
```

- `port`: the engine's `--debug-port` (default 6510).
- `localRoot`: the project directory as VS Code sees it. Only needed when it
  differs from the path the engine was started with.

Stopping the debug session detaches; the game keeps running.

## Test

```bash
npm test
```
