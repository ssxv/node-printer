# [@ssxv/node-printer](https://www.npmjs.com/package/@ssxv/node-printer)

**Native Node.js printing library for Windows and Linux**

Cross-platform printing addon that handles the OS-specific complexity for you. Print PDFs, text files, and raw data to any system printer with a simple JavaScript API.

## What is this?

A Node.js addon that provides:

- **Cross-platform printing**: Works on Windows (Winspool) and Linux (CUPS)
- **JS-first API**: Hide OS printing complexity behind clean JavaScript functions
- **File and raw printing**: Handle documents (PDFs, text) and direct printer control (receipts, labels)
- **Async job management**: Submit, monitor, and cancel print jobs

## Why use this library?

Cross-platform printing in Node.js is notoriously difficult:

- **Existing solutions are incomplete**: Most libraries only work on Windows or are abandoned
- **OS printing APIs are complex**: Different systems, different APIs, different quirks
- **Production requirements**: You need reliable printing that works in headless servers and kiosks

This library solves those problems by providing a unified JavaScript interface backed by native Windows and Linux implementations.

## Installation

```bash
npm install @ssxv/node-printer
```

**Requirements:**
- Node.js 18.x, 20.x, 22.x, or 24.x
- Windows: Works out of the box
- Linux: Requires CUPS (`sudo apt-get install libcups2-dev` for building from source)

Prebuilt binaries are available for common platforms. Falls back to source build if needed.

## Quick Start

### List available printers

```javascript
const { printers } = require('@ssxv/node-printer');

// Get all printers
const allPrinters = await printers.list();
console.log(allPrinters);

// Get default printer
const defaultPrinter = await printers.default();
console.log('Default:', defaultPrinter.name);
```

### Print a PDF file

```javascript
const { jobs } = require('@ssxv/node-printer');

// Submit print job
const job = await jobs.printFile({
  printer: 'HP LaserJet Pro',
  file: './sample.pdf',
});
console.log('Job submitted:', job.id);

// Check job status
const jobInfo = await jobs.get('HP LaserJet Pro', job.id);
console.log('Job state:', jobInfo.state);
```

### Print raw data (receipt printer example)

```javascript
const { jobs } = require('@ssxv/node-printer');

// ESC/POS commands for receipt printer
const receiptData = Buffer.from([
  0x1B, 0x40,           // Initialize printer
  ...Buffer.from('RECEIPT\n'),
  0x1B, 0x64, 0x02      // Cut paper
]);

const job = await jobs.printRaw({
  printer: 'EPSON-TM88V',
  data: receiptData,
});
```

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

## API Overview

### Printers

- `printers.list()` - Get all available printers
- `printers.get(name)` - Get specific printer details
- `printers.default()` - Get default system printer
- `printers.capabilities(name)` - Get printer capabilities
- `printers.driverOptions(name)` - Get available print options

### Jobs

- `jobs.printFile({ printer, file, ...options })` - Print documents (PDF, text, etc.)
- `jobs.printRaw({ printer, data, ...options })` - Send raw data directly to printer
- `jobs.get(printer, jobId)` - Get job status and details
- `jobs.list(printer)` - List all jobs for a printer
- `jobs.cancel(printer, jobId)` - Cancel a specific job
- `jobs.setNative(printer, jobId, options)` - Set native print options

## Important Notes

**Job submission ≠ job completion**: `printFile` and `printRaw` return immediately after submitting the job to the system print spooler. The actual printing happens asynchronously. Use `jobs.get()` to monitor job progress.

**Choose the right function**:
- Use `printFile` for documents (PDFs, text files, images)
- Use `printRaw` for direct printer control (receipt printers, label printers, ESC/POS commands)

## Documentation

For complete documentation, advanced usage, platform-specific notes, and troubleshooting, visit the **[Wiki](https://github.com/ssxv/node-printer/wiki)**.

## Development

```bash
# Install dependencies
npm install

# Build native addon
npm run build
```

**Build requirements:**
- Windows: Visual Studio Build Tools + Python
- Linux: `libcups2-dev build-essential python3`

## Contact

- Issues & PRs: https://github.com/ssxv/node-printer
