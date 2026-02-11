// TypeScript type definitions for @ssxv/node-printer

export interface Printer {
  name: string;
  isDefault: boolean;
  state: 'idle' | 'printing' | 'stopped' | 'offline' | 'error';
  location?: string;
  description?: string;
}

export interface PrinterCapabilities {
  formats: ('PDF' | 'TEXT' | 'RAW' | 'IMAGE')[];
  paperSizes?: string[];
  duplex?: boolean;
  color?: boolean;
}

export interface PrintJob {
  id: number;
  state: 'pending' | 'printing' | 'completed' | 'canceled' | 'paused' | 'error';
  printer?: string;
  title?: string;
  user?: string;
  creationTime?: Date;
  processingTime?: Date;
  completedTime?: Date;
  pages?: number;
  size?: number;
}

export interface PrintFileOptions {
  printer: string;
  file: string;
  options?: PrintOptions;
}

export interface PrintRawOptions {
  printer: string;
  data: Buffer;
  format?: 'RAW';
  options?: PrintOptions;
}

export interface PrintOptions {
  copies?: number;
  duplex?: boolean;
  color?: boolean;
  paperSize?: string;
  orientation?: 'portrait' | 'landscape';
  jobName?: string;
}

export interface PrintJobResult {
  id: number;
  printer: string;
}

export interface PrinterDriverOptions {
  [key: string]: any;
}
