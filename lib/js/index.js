"use strict";
// Main entry point for @ssxv/node-printer
// JS-first, cross-platform Node.js printing library
Object.defineProperty(exports, "__esModule", { value: true });
exports.printFile = exports.printDirect = exports.getPrinter = exports.getPrinters = exports.PrinterError = exports.jobs = exports.printers = void 0;
// New JS-first API exports
var printers_1 = require("./printers");
Object.defineProperty(exports, "printers", { enumerable: true, get: function () { return printers_1.printers; } });
var jobs_1 = require("./jobs");
Object.defineProperty(exports, "jobs", { enumerable: true, get: function () { return jobs_1.jobs; } });
var errors_1 = require("./errors");
Object.defineProperty(exports, "PrinterError", { enumerable: true, get: function () { return errors_1.PrinterError; } });
// Import for default export
const printers_2 = require("./printers");
const jobs_2 = require("./jobs");
const errors_2 = require("./errors");
// Legacy API compatibility - import from existing wrapper
const legacy = require('../../printer');
// Backward compatibility exports (deprecated but maintained)
// Only the 4 core legacy functions are supported
/**
 * @deprecated Use printers.list() instead
 */
exports.getPrinters = legacy.getPrinters;
/**
 * @deprecated Use printers.get(name) instead
 */
exports.getPrinter = legacy.getPrinter;
/**
 * @deprecated Use jobs.printRaw() instead
 */
exports.printDirect = legacy.printDirect;
/**
 * @deprecated Use jobs.printFile() instead
 */
exports.printFile = legacy.printFile;
// Default export maintains compatibility
exports.default = {
    // New API
    printers: printers_2.printers,
    jobs: jobs_2.jobs,
    PrinterError: errors_2.PrinterError,
    // Legacy API (deprecated) - Only 4 core functions
    getPrinters: exports.getPrinters,
    getPrinter: exports.getPrinter,
    printDirect: exports.printDirect,
    printFile: exports.printFile
};
//# sourceMappingURL=index.js.map