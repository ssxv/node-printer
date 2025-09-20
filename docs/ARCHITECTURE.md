# Architecture & Data Flow

This document describes the layering and data flow for the `node-printer` project.

```
Application (your JS)      // calls into the public API
      |
      v
printer.js (JS wrapper)    // normalizes args/results, promise/callback wiring
      |
      v
index.js (binding loader)  // loads native binding (.node) via build/Release or prebuild-install
      |
      v
node_printer_win.cc (.node native addon)
  - Implements: getPrinters(), printDirect(), printFile()
  - Uses N-API to convert JS <-> native types
  - Uses Win32 APIs (EnumPrinters, StartDocPrinter, WritePrinter, etc.)
      |
      v
Windows Print Subsystem (Winspool)  // actual OS printing services and drivers

Data flows down: JS args -> native (buffers/filenames) -> Winspool calls
Results/errors flow up: Winspool status/errors -> native -> JS (Error objects / normalized results)
```

Notes:
- `index.js` is a loader and does not implement printing logic; it ensures the correct native `.node` binary is loaded.
- The native addon uses N-API and runs heavy or blocking I/O on AsyncWorker threads to avoid blocking Node's event loop.
- Unit tests use a JS mock for the native binding so unit tests run deterministically on non-Windows CI.

See also: `index.md` (project documentation entrypoint)

For more detailed flows (e.g., `printDirect` small-buffer vs streaming via temp-file), see the native worker implementation in `src/node_printer_win.cc`.
