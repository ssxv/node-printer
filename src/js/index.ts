// Main entry point for @ssxv/node-printer
// JS-first, cross-platform Node.js printing library

// New JS-first API exports
export { printers } from './printers';
export { jobs } from './jobs';
export { PrinterError } from './errors';

// Import for default export
import { printers } from './printers';
import { jobs } from './jobs';
import { PrinterError } from './errors';

// Re-export types for convenience
export type {
  Printer,
  PrinterCapabilities,
  JobStatus,
  PrintFileOptions,
  PrintRawOptions,
  PrintOptions,
  PrintJobResult,
  PrinterErrorCode,
  // Legacy types for backward compatibility
  PrinterDetails,
  PrintDirectOptions,
  PrinterDriverOptions
} from './types';

// Legacy API compatibility - import from existing wrapper
const legacy = require('../../printer');

// Backward compatibility exports (deprecated but maintained)
// Only the 4 core legacy functions are supported
/**
 * @deprecated Use printers.list() instead
 */
export const getPrinters = legacy.getPrinters;

/**
 * @deprecated Use printers.get(name) instead  
 */
export const getPrinter = legacy.getPrinter;

/**
 * @deprecated Use jobs.printRaw() instead
 */
export const printDirect = legacy.printDirect;

/**
 * @deprecated Use jobs.printFile() instead  
 */
export const printFile = legacy.printFile;

// Default export maintains compatibility
export default {
  // New API
  printers: printers,
  jobs: jobs,
  PrinterError: PrinterError,
  
  // Legacy API (deprecated) - Only 4 core functions
  getPrinters,
  getPrinter,
  printDirect,
  printFile
};