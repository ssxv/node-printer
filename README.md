# node-printer

Proof-of-concept N-API addon exposing basic printing APIs on Windows (Winspool).

This repository demonstrates building a small native Node.js addon with `node-addon-api` and provides a JavaScript wrapper with Promise and callback-compatible interfaces.

Documentation is under the `docs/` folder. Start at `docs/index.md` for an overview and links.

Supported Node.js versions
- Node.js 18.x, 20.x, 22.x

Quick install (Windows development)

```powershell
# Install dev deps
npm ci
# Build native addon (requires Visual Studio Build Tools + Python)
npm run build
# Run unit tests (deterministic / mock-driven)
npm test
```

Usage (example)

```javascript
const printer = require('./printer');

async function list() {
  const printers = await printer.getPrinters();
  console.log(printers);
}

list();
```

Testing & mocks
- Unit tests are mock-driven and deterministic; unit tests load a JS mock for the native binding so they run on non-Windows CI.
- Integration/native tests are gated. To run integration tests on Windows set `RUN_INTEGRATION=1` in the environment and ensure a test printer is available.

Prebuilds and publishing
- To provide painless installs for Windows users, publish prebuilt binaries (per Node ABI and architecture) and keep the `install` script in `package.json` as `prebuild-install || node-gyp rebuild` so installations prefer prebuilt binaries and fall back to source builds.
- CI should build and attach prebuild artifacts to GitHub Releases for each supported Node.js version and architecture you want to support.

Documentation
- Start here: `docs/index.md`
- Architecture & data flow: `docs/ARCHITECTURE.md`
- Type definitions: `types/index.d.ts`

Notes
- This PoC implements Windows (Winspool) functionality only. POSIX/CUPS support is planned separately.
- Consider adding `"os": ["win32"]` to `package.json` to restrict installs to Windows if appropriate.

Contact
- Issues & PRs: https://github.com/ssxv/node-printer
