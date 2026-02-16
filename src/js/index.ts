// Main entry point for @ssxv/node-printer

import { printers } from './printers';
import { jobs } from './jobs';
import { network } from './network';
import { PrinterError } from './errors';

// Named exports
export { printers, jobs, network, PrinterError };

// Re-export types for convenience
export type {
  Printer,
  PrinterCapabilities,
  PrintJob,
  PrintFileOptions,
  PrintRawOptions,
  PrintSocketOptions,
  PrintOptions,
  PrintJobResult,
  PrinterDriverOptions
} from './types';

// Default export - modern API only
export default {
  printers,
  jobs,
  network,
  PrinterError
};
