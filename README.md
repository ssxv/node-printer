# [@ssxv/node-printer](https://www.npmjs.com/package/@ssxv/node-printer)

Cross-platform Node.js native printing bindings (Windows & POSIX/CUPS)

`@ssxv/node-printer` is a JS-first, cross-platform Node.js printing library implemented as an N-API native addon. Originally a Windows (Winspool) proof-of-concept, it has been comprehensively refactored into a modern, enterprise-grade printing solution with unified APIs for both Windows and Linux systems.

## Key features
- **JS-first Architecture**: Modern TypeScript-based API with compile-time type safety
- **Streamlined Legacy API**: 4 core functions for backward compatibility:
  - `getPrinters` - List available printers
  - `getPrinter` - Get specific printer details
  - `printFile` - Print files with flexible options
  - `printDirect` - Direct raw data printing
- **Enhanced Error Handling**: Unified error normalization across Windows and POSIX platforms
- **Dual platform support**: Win32 (Winspool) and POSIX (libcups) with shared abstraction layer
- **Flexible API Options**:
  - Legacy callback/promise API (`require('@ssxv/node-printer/printer')`)
  - Modern JS-first API (`require('@ssxv/node-printer')`)
- **Cross-platform Error Codes**: Standardized error types for consistent error handling

## Architecture

This library features a 5-phase modern architecture:

1. **TypeScript Structure**: JS-first API with compile-time type safety
2. **Platform Separation**: Dedicated Windows (WinSpool) and POSIX (CUPS) implementations
3. **Native Abstraction Layer**: Unified C++ interface with 10 core functions
4. **Error Normalization**: Cross-platform error mapping with enhanced error information
5. **Backward Compatibility**: Streamlined legacy API maintaining zero breaking changes

### Error Handling

The library provides enhanced error handling with standardized error codes:

- `PRINTER_NOT_FOUND` - Printer doesn't exist or unavailable
- `PRINTER_OFFLINE` - Printer offline or stopped  
- `ACCESS_DENIED` - Permission errors
- `JOB_NOT_FOUND` - Print job not found
- `DRIVER_ERROR` - Driver/PPD related issues
- `FILE_NOT_FOUND` - File not accessible
- `INVALID_ARGUMENTS` - Invalid parameters
- `UNSUPPORTED_FORMAT` - Format not supported
- `UNKNOWN` - Fallback category

Errors include platform-specific details for enhanced debugging while maintaining cross-platform consistency.

## API Reference

### Legacy API (printer.js)
- `getPrinters()` → `Promise<PrinterInfo[]>`
- `getPrinter(name?)` → `Promise<PrinterInfo>` 
- `printFile(options)` → `Promise<number>`
- `printDirect(options)` → `Promise<number>`

### Modern API (lib/js)
- `printers.list()` → `Promise<Printer[]>`
- `printers.get(name)` → `Promise<Printer>`
- `printers.getDefault()` → `Promise<Printer>`
- `jobs.printFile(filename, printer, options?)` → `Promise<number>`
- `jobs.printRaw(data, printer, options?)` → `Promise<number>`
- `jobs.get(printer, jobId)` → `Promise<JobInfo>`
- Plus additional functions for complete printing workflow management

Documentation is under the `docs/` folder. Start at `docs/index.md` for an overview and links; the architecture overview is in `docs/ARCHITECTURE.md`.

## Supported Node.js versions
- Node.js 18.x, 20.x, 22.x, 24.x

## Quick start — Development (Windows)

```powershell
# Install dev dependencies
npm ci
# Build native addon (requires Visual Studio Build Tools + Python)
npm run build
# Run unit tests (mock-driven)
npm test
```

## Quick start — Development (Linux / WSL)

```bash
# Install system build deps (Ubuntu/Debian)
sudo apt-get update; sudo apt-get install -y libcups2-dev build-essential python3
# Install node deps and build
npm ci
npm run build
# Run unit tests (mock-driven)
npm test
```

## Usage examples

### Legacy API (Backward Compatible)
```javascript
const printer = require('@ssxv/node-printer/printer');

// List all printers
const printers = await printer.getPrinters();
console.log(printers);

// Get specific printer
const myPrinter = await printer.getPrinter('MyPrinter');

// Print a file
await printer.printFile({
  filename: 'document.pdf',
  printer: 'MyPrinter'
});

// Direct raw printing
await printer.printDirect({
  data: Buffer.from('Hello World'),
  printer: 'MyPrinter',
  type: 'RAW'
});
```

### Modern JS-first API (Enhanced Features)
```javascript
const { printers, jobs, PrinterError } = require('@ssxv/node-printer');

// List printers with enhanced info
const printerList = await printers.list();

// Get default printer
const defaultPrinter = await printers.getDefault();

// Print with enhanced options
try {
  const jobId = await jobs.printFile('document.pdf', 'MyPrinter', {
    copies: 2,
    paperSize: 'A4'
  });
  console.log('Print job started:', jobId);
} catch (error) {
  if (error instanceof PrinterError) {
    console.log('Print error:', error.code, error.message);
  }
}

// Get job status
const jobInfo = await jobs.get('MyPrinter', jobId);
```

## Prebuilt binaries & publishing
- The repository includes a workflow `.github/workflows/prebuild-and-publish.yml` which builds prebuilt binaries on Windows and Linux runners for supported Node.js versions and uploads them as release artifacts. The install flow prefers prebuilt binaries and falls back to source builds using the `install` script in `package.json` (`prebuild-install || node-gyp rebuild`).
- If you maintain a package registry release, ensure CI publishes prebuilt artifacts for all target Node.js versions and architectures you intend to support.

## Build notes & troubleshooting

- Windows: Visual Studio Build Tools (MSVC) + Python are required to compile the native addon. Follow the Node.js native build tooling docs for setting up `node-gyp` on Windows.
- Linux: `libcups2-dev` (or the distribution equivalent) is required to build the POSIX/CUPS native sources. On Debian/Ubuntu: `sudo apt-get install libcups2-dev build-essential python3`.
- If `prebuild-install` cannot find a compatible prebuilt binary during `npm install`, the package will fall back to a source build and you will need the platform build toolchain installed.

## Notes on CUPS API requirement and driver option parsing

- Minimum libcups requirement: This project requires the destination-based CUPS APIs introduced in CUPS 1.6. Practically, that means the build environment must provide the modern destination APIs (for example, `cupsCopyDestInfo`) via the development headers (`libcups2-dev` or equivalent). In short: libcups >= 1.6 is required.

- Rationale: The PPD helper family (e.g., `cupsGetPPD`, `ppdOpenFile`, `ppdMarkDefaults`, `cupsMarkOptions`, `ppdClose`) were deprecated in CUPS 1.6 in favor of destination-based APIs. Requiring the modern APIs avoids deprecated behavior, gives richer and more consistent printer metadata, and simplifies the native implementation.

- Verification commands (run on Linux/WSL to confirm headers and version):

```bash
# Check libcups pkg-config version (if available)
pkg-config --modversion libcups || pkg-config --modversion cups

# Search for destination API symbol in headers
grep -nR "cupsCopyDestInfo" /usr/include 2>/dev/null || echo 'cupsCopyDestInfo not found in /usr/include'

# Search for deprecated PPD helper symbols (optional)
grep -nR "cupsGetPPD\|ppdOpenFile\|ppdMarkDefaults\|cupsMarkOptions\|ppdClose" /usr/include 2>/dev/null || echo 'ppd helper symbols not found in /usr/include'
```

- CI note: Update your CI runners to install the CUPS development package (for example `libcups2-dev` on Debian/Ubuntu) before building and running integration tests. The repository's integration tests that exercise POSIX bindings require those headers/libraries.

- Compatibility guidance: By design, this repository requires destination-based CUPS APIs (libcups >= 1.6). If you need to support very old systems that lack these APIs, consider one of the following:
  - Provide a compatibility branch or patch that implements a fallback using older PPD helpers (note: those APIs are deprecated and brittle).
  - Ship prebuilt binaries for the older target environments you need to support.
  - Encourage users on older systems to upgrade libcups or use a newer distribution image.

## Contact
- Issues & PRs: https://github.com/ssxv/node-printer
