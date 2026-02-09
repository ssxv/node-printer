// Minimal mock for the native binding to allow JS-level tests on non-Windows
module.exports = {
  getPrinters: () => [],
  getDefaultPrinterName: () => 'Mock Default Printer',
  printDirect: function(opts) {
    // Support both object and positional signatures:
    // - printDirect(opts)
    // - printDirect(data, printer, docname, type)
    let printer = null;
    if (opts && typeof opts === 'object' && !Buffer.isBuffer(opts)) {
      printer = opts.printer;
    } else if (Buffer.isBuffer(opts) || typeof opts === 'string') {
      // positional form: first arg is data; next arg may be printer
      const args = Array.from(arguments);
      printer = args[1];
    }
    // emulate promise-based API used in wrapper: reject for invalid printer
    return new Promise((resolve, reject) => {
      if (printer === 'invalid' || printer === 'Printer-Does-Not-Exist-XYZ-123') {
        const e = new Error('mock printer invalid');
        e.code = 1234;
        return reject(e);
      }
      // emulate success with job id 42
      return resolve(42);
    });
  },
  printFile: (filename, docname, printer) => {
    // Support positional signature and synchronous throw when file cannot be opened
    // If called with an object-style first arg, handle accordingly
    if (typeof filename === 'object' && filename !== null) {
      // args: { filename, ... }
      const opts = filename;
      filename = opts.filename;
      docname = opts.docname;
      printer = opts.printer;
    }

    // Synchronous behavior: if filename can't be opened, throw synchronously
    if (typeof filename === 'string' && filename === 'non-existent-file.xyz') {
      const e = new Error('cannot open file: ' + filename);
      e.code = 0;
      throw e; // synchronous throw as native binding is expected to do
    }

    // For existing files, return a Promise; reject if printer invalid
    return new Promise((resolve, reject) => {
      if (printer === 'Printer-Does-Not-Exist-XYZ-123' || printer === 'invalid') {
        const e = new Error('mock printer invalid');
        e.code = 1234;
        return reject(e);
      }
      return resolve(99);
    });
  }
};