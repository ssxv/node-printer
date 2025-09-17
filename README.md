Proof-of-concept N-API addon for getPrinters (Windows-only)

This small PoC demonstrates using `node-addon-api` to export a `getPrinters` function on Windows.

Requirements (Windows):

# node-printer — Windows PoC (N-API)

Proof-of-concept N-API addon exposing basic printing APIs on Windows (Winspool). This repository demonstrates using `node-addon-api` to build a small native addon and provides a JS wrapper with Promise and callback-compatible interfaces.

## Current status
- JS wrapper and unit tests: stable — unit tests are mock-driven and deterministic.
- Native code: present and builds with `node-gyp` on Windows, but prebuilt binaries are not published yet.
- Integration/native tests: gated and must be run on Windows with a configured test printer (set `RUN_INTEGRATION=1`).

## Supported Node versions
This package supports Node.js runtimes 18.x, 20.x and 22.x only. Node 14 has been removed due to End-of-Life and modern tooling requirements.

## Quick start (development on Windows)
```powershell
npm ci
npm run build    # requires Visual Studio Build Tools + Python for node-gyp
npm test         # runs unit tests; tests own mocking
```

## Testing and mocking
- Unit tests explicitly own the mock binding. Tests that exercise the JS wrapper preload the mock into Node's `require` cache before requiring `../printer` so the wrapper picks up the mock without relying on environment variables.
- Do not rely on `USE_MOCK` to control test behavior; that environment variable is no longer required.
- Integration/native tests are gated with `RUN_INTEGRATION=1` and should only be executed on Windows runners that have a test or virtual printer configured.

Example: run native integration tests locally (Windows with test printer):
```powershell
- When publishing, ensure `build/Release/node_printer.node` is built for your target environment or use prebuilds.

```

## Prebuilds and publishing (recommended)
To make installs seamless for Windows consumers, publish prebuilt binaries for each supported Node version and CPU architecture. Recommended flow:

1. Build prebuilds in CI (use `prebuild` or `prebuild --backend=node-gyp`) for each target Node version/arch.
2. Upload the generated artifacts to a GitHub Release (CI can automate this).
3. Keep `install` as `prebuild-install || node-gyp rebuild` in `package.json` so installs prefer prebuilt binaries and fall back to source build.

Local prebuild example (single target):
```powershell
npm ci
npx prebuild --backend=node-gyp --strip
# artifacts appear under ./prebuilds
```

## CI recommendations
- Add a GitHub Actions job with a Windows matrix for Node versions you intend to support (e.g., 18, 20, 22) and architectures (x64, arm64 if required).
	- Note: this repository supports Node 18/20/22 only; do not include Node 14 in CI or prebuild matrices.
- For each runner: build prebuilds, run unit tests, and attach prebuild artifacts to the Release.
- Optionally run integration tests on a dedicated Windows runner with a test printer (set `RUN_INTEGRATION=1` only when a test printer is available).

## Packaging guidance
- Avoid shipping an unqualified `build/Release/node_printer.node` inside `files` unless you intentionally want to publish that specific binary. Prefer publishing per-ABI prebuilds instead.
- To restrict installs to Windows only (optional), add to `package.json`:
```json
"os": ["win32"]
```

## Publishing
When prebuild artifacts are attached to a GitHub Release, publish the npm package as usual:
```powershell
# ensure release includes prebuild artifacts
npm publish --access public
```

If you do not publish prebuilds, document that consumers require a native toolchain (Visual Studio Build Tools + Python) to install from source.

## Notes & Roadmap
- This PoC implements Windows (Winspool) functionality only. POSIX/CUPS support is planned separately.
- The module exposes both Promise and callback-compatible APIs. Type definitions are available in `types/index.d.ts`.
- I can add a CI workflow that produces prebuild artifacts and uploads them to Releases (recommended prior to publishing). If you want, I can implement that next.

## Contact
- Issues & PRs: https://github.com/ssxv/node-printer
