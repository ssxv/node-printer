Proof-of-concept N-API addon for getPrinters (Windows-only)

This small PoC demonstrates using `node-addon-api` to export a `getPrinters` function on Windows.

Requirements (Windows):

Build:

```powershell
npm install
npm run build
```

Notes:
node-printer
============

A modern Proof-of-Concept N-API-based Node.js addon that exposes printer functionality on Windows.

This repository contains a Windows-only proof-of-concept (`node-printer`) that demonstrates:

- Using `node-addon-api` to create an ABI-stable native addon.
- Returning rich printer objects (names, options, jobs) from native code.
- Non-blocking printing functions implemented with `Napi::AsyncWorker` that return Promises.

Quick start
-----------

Build (Windows, requires Visual Studio Build Tools and Python for node-gyp):

```powershell
npm install
npm run build
```

Run smoke tests:

```powershell
npm test
```

Usage
-----

```js
// when installed from npm
const printer = require('@ssxv/node-printer');

// Promise style
await printer.printFile({ filename: 'test.pdf', printer: 'MyPrinter' });

// Callback style
printer.printDirect({ data: 'raw', printer: 'MyPrinter' }, (err, jobId) => {
	if (err) console.error(err);
	else console.log('job id', jobId);
});
```

Publishing
----------

The repository is configured to publish a scoped package (`@ssxv/node-printer`). To publish a release from GitHub Actions the workflow runs `npm publish --access public`. Locally you can publish with:

```powershell
# make sure you're logged in to npm and have the right permissions
npm publish --access public
```

Notes
-----
- This PoC currently implements only Windows (Winspool) functionality. POSIX/CUPS support is planned.
- The module exports both Promise and callback-compatible APIs for `printDirect` and `printFile`.
- When publishing, ensure `build/Release/node_printer.node` is built for your target environment or use prebuilds.
