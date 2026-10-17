// @ts-check
'use strict';

// Kept free of the vscode module so it can be tested with plain Node.

const DEFAULT_PORT = 6510;
// The engine only listens on the loopback address (DAP can run code).
const HOST = '127.0.0.1';

/**
 * Fills in and checks an attach configuration. An empty configuration is what
 * VS Code passes when F5 is pressed without a launch.json.
 *
 * @param {Record<string, any>} config
 * @returns {{ config: Record<string, any> } | { error: string }}
 */
function resolveAttach(config) {
	const resolved = { ...config };
	if (!resolved.type && !resolved.request && !resolved.name) {
		resolved.type = 'sowa';
		resolved.request = 'attach';
		resolved.name = 'Attach to Sowa';
	}
	if (resolved.request !== 'attach') {
		return { error: 'Sowa is debugged by attaching to a running editor or game ("request": "attach").' };
	}
	if (resolved.port === undefined) {
		resolved.port = DEFAULT_PORT;
	}
	if (!Number.isInteger(resolved.port) || resolved.port < 1 || resolved.port > 65535) {
		return { error: `"port" must be a whole number from 1 to 65535, not ${JSON.stringify(resolved.port)}.` };
	}
	if (resolved.localRoot !== undefined && typeof resolved.localRoot !== 'string') {
		return { error: '"localRoot" must be a path.' };
	}
	return { config: resolved };
}

/**
 * Where the debug adapter for a resolved configuration listens.
 *
 * @param {Record<string, any>} config
 */
function adapterAddress(config) {
	return { host: HOST, port: config.port };
}

module.exports = { DEFAULT_PORT, HOST, resolveAttach, adapterAddress };
