// Main entry point for @ssxv/node-printer

import { printers } from './printers';
import { jobs } from './jobs';
import { PrinterError } from './errors';

// Named exports
export { printers, jobs, PrinterError };

// Re-export types for convenience
export type {
  Printer,
  PrinterCapabilities,
  PrintJob,
  PrintFileOptions,
  PrintRawOptions,
  PrintOptions,
  PrintJobResult,
  PrinterDriverOptions
} from './types';

// Default export - modern API only
export default {
  printers,
  jobs,
  PrinterError
};
