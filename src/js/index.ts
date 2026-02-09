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

// Default export - modern API only
export default {
  printers: printers,
  jobs: jobs,
  PrinterError: PrinterError
};