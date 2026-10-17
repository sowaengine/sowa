// @ts-check
'use strict';

const vscode = require('vscode');
const { resolveAttach, adapterAddress } = require('./configuration');

/** @param {vscode.ExtensionContext} context */
function activate(context) {
	context.subscriptions.push(
		vscode.debug.registerDebugConfigurationProvider('sowa', {
			resolveDebugConfiguration(_folder, config) {
				const result = resolveAttach(config);
				if ('error' in result) {
					vscode.window.showErrorMessage(result.error);
					// undefined aborts the session without VS Code's own error.
					return undefined;
				}
				return /** @type {vscode.DebugConfiguration} */ (result.config);
			},
		}),
		// The engine is the debug adapter: VS Code talks DAP to it directly.
		vscode.debug.registerDebugAdapterDescriptorFactory('sowa', {
			createDebugAdapterDescriptor(session) {
				const { host, port } = adapterAddress(session.configuration);
				return new vscode.DebugAdapterServer(port, host);
			},
		}),
	);
}

function deactivate() {}

module.exports = { activate, deactivate };
