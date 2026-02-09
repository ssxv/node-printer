// Legacy native binding loader for @ssxv/node-printer/printer
// Strategy:
// 1. Try to require the locally-built addon: build/Release/node_printer_legacy.node
// 2. If that fails, try `prebuild-install` to fetch a prebuilt binary
// 3. If still not found, try to fallback to `node-gyp rebuild` (requires build toolchain)
// 4. Export whatever native binding we obtain (or throw an informative error)

const path = require('path');
const fs = require('fs');

function tryRequireBuilt() {
	const builtPath = path.join(__dirname, 'build', 'Release', 'node_printer_legacy.node');
	if (fs.existsSync(builtPath)) {
		return require(builtPath);
	}
	return null;
}

function tryPrebuildInstall() {
	try {
		// prebuild-install will attempt to download a matching prebuilt binary into the package
		// and then we can require the same path as above.
		const prebuildInstall = require('prebuild-install');
		// Call prebuild-install programmatically
		// Note: this call exits the process on failure when run as CLI; here we use programmatic API.
		prebuildInstall.prebuild({ module_name: 'node_printer_legacy', module_path: './build/Release' });
		const built = tryRequireBuilt();
		if (built) return built;
	} catch (e) {
		// ignore and fallback
	}
	return null;
}

function tryNodeGypRebuild() {
	try {
		// Attempt an in-process rebuild via node-gyp. This is best-effort and requires a native toolchain.
		// Use child_process.spawnSync to run `node-gyp rebuild`.
		const child = require('child_process');
		const res = child.spawnSync(process.execPath, [require.resolve('node-gyp/bin/node-gyp'), 'rebuild', '--target=node_printer_legacy'], {
			stdio: 'inherit',
			cwd: __dirname
		});
		if (res.status === 0) {
			return tryRequireBuilt();
		}
	} catch (e) {
		// ignore
	}
	return null;
}

function loadBinding() {
	// 1. local build
	let binding = tryRequireBuilt();
	if (binding) return binding;

	// 2. prebuild-install
	binding = tryPrebuildInstall();
	if (binding) return binding;

	// 3. node-gyp rebuild
	binding = tryNodeGypRebuild();
	if (binding) return binding;

	// 4. cannot load
	throw new Error('Failed to load legacy node_printer binding. Tried build/Release/node_printer_legacy.node, prebuild-install, and node-gyp rebuild. Ensure native build succeeded or install prebuilt binaries.');
}

module.exports = loadBinding();