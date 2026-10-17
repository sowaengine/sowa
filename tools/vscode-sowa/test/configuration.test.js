'use strict';

const test = require('node:test');
const assert = require('node:assert/strict');
const { DEFAULT_PORT, resolveAttach, adapterAddress } = require('../configuration');

test('F5 without a launch.json attaches on the default port', () => {
	const result = resolveAttach({});
	assert.deepEqual(result, {
		config: { type: 'sowa', request: 'attach', name: 'Attach to Sowa', port: DEFAULT_PORT },
	});
	assert.equal(DEFAULT_PORT, 6510);
});

test('A configuration keeps its own port and localRoot', () => {
	const config = { type: 'sowa', request: 'attach', name: 'Game', port: 7000, localRoot: '/p' };
	assert.deepEqual(resolveAttach(config), { config });
});

test('A configuration without a port gets the default one', () => {
	const result = resolveAttach({ type: 'sowa', request: 'attach', name: 'Game' });
	assert.equal(result.config.port, DEFAULT_PORT);
});

test('Launch is refused', () => {
	const result = resolveAttach({ type: 'sowa', request: 'launch', name: 'Game' });
	assert.match(result.error, /attach/);
});

test('A bad port or localRoot is refused', () => {
	for (const port of [0, 65536, 1.5, '6510']) {
		const result = resolveAttach({ type: 'sowa', request: 'attach', name: 'Game', port });
		assert.ok('error' in result, `port ${JSON.stringify(port)}`);
	}
	const result = resolveAttach({ type: 'sowa', request: 'attach', name: 'Game', localRoot: 3 });
	assert.ok('error' in result);
});

test('The adapter is reached on the loopback address', () => {
	assert.deepEqual(adapterAddress({ port: 7000 }), { host: '127.0.0.1', port: 7000 });
});
