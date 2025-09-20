# @ssxv/node-printer

Cross-platform Node.js native printing bindings (Windows & POSIX/CUPS)

`@ssxv/node-printer` is an N-API native addon exposing a small set of printing APIs to Node.js. The project started as a Windows (Winspool) proof-of-concept and has been extended to include POSIX (CUPS) support so the addon can be built and used on both Windows and Linux systems.

Key features
- Exports: `getPrinters`, `getPrinter`, `getJob`, `getPrinterDriverOptions`, `printFile`, `printDirect`, `setJob`, `getDefaultPrinterName`, `getSupportedPrintFormats`, and `getSupportedJobCommands`.
- Dual platform support: Win32 (Winspool) and POSIX (libcups).
- Promise and callback-friendly JavaScript wrapper (`printer.js`).
- Unit tests are deterministic and mock-driven; integration tests exercise native bindings on the host platform.

Documentation is under the `docs/` folder. Start at `docs/index.md` for an overview and links; the architecture overview is in `docs/ARCHITECTURE.md`.

Supported Node.js versions
- Node.js 18.x, 20.x, 22.x

Quick start — Development (Windows)

```powershell
# Install dev dependencies
npm ci
# Build native addon (requires Visual Studio Build Tools + Python)
npm run build
# Run unit tests (mock-driven)
npm test
```

Quick start — Development (Linux / WSL)

```bash
# Install system build deps (Ubuntu/Debian)
sudo apt-get update; sudo apt-get install -y libcups2-dev build-essential python3
# Install node deps and build
npm ci
npm run build
# Run unit tests (mock-driven)
npm test
```

Usage example

```javascript
const printer = require('./printer');

async function list() {
  const printers = await printer.getPrinters();
  console.log(printers);
}

list();
```

Prebuilt binaries & publishing
- The repository includes a workflow `.github/workflows/prebuild-and-publish.yml` which builds prebuilt binaries on Windows and Linux runners for supported Node.js versions and uploads them as release artifacts. The install flow prefers prebuilt binaries and falls back to source builds using the `install` script in `package.json` (`prebuild-install || node-gyp rebuild`).
- If you maintain a package registry release, ensure CI publishes prebuilt artifacts for all target Node.js versions and architectures you intend to support.

Build notes & troubleshooting
- Windows: Visual Studio Build Tools (MSVC) + Python are required to compile the native addon. Follow the Node.js native build tooling docs for setting up `node-gyp` on Windows.
- Linux: `libcups2-dev` (or the distribution equivalent) is required to build the POSIX/CUPS native sources. On Debian/Ubuntu: `sudo apt-get install libcups2-dev build-essential python3`.
- If `prebuild-install` cannot find a compatible prebuilt binary during `npm install`, the package will fall back to a source build and you will need the platform build toolchain installed.

Notes on driver option parsing (CUPS)
- The POSIX implementation retrieves driver/option information from libcups. Historically the project used deprecated PPD helper APIs; the codebase has been migrated to use non-deprecated libcups APIs and, where available, `cupsCopyDestInfo`-style calls to gather richer printer driver metadata. Builds against older libcups that lack newer APIs will fall back to a conservative mapping of the destination options (name → value).

Contact
- Issues & PRs: https://github.com/ssxv/node-printer
