// Error types for @ssxv/node-printer
// Cross-platform error normalization

export type PrinterErrorCode =
  | 'PRINTER_NOT_FOUND'
  | 'PRINTER_OFFLINE'
  | 'ACCESS_DENIED'
  | 'JOB_NOT_FOUND'
  | 'DRIVER_ERROR'
  | 'INVALID_ARGUMENTS'
  | 'FILE_NOT_FOUND'
  | 'UNSUPPORTED_FORMAT'
  | 'UNKNOWN';

export class PrinterError extends Error {
  public readonly code: PrinterErrorCode;
  public readonly originalError?: any;

  constructor(message: string, code: PrinterErrorCode = 'UNKNOWN', originalError?: any) {
    super(message);
    this.name = 'PrinterError';
    this.code = code;
    this.originalError = originalError;

    // Maintains proper stack trace for where error was thrown (only on V8)
    if (Error.captureStackTrace) {
      Error.captureStackTrace(this, PrinterError);
    }
  }

  static fromNativeError(nativeError: any): PrinterError {
    if (nativeError instanceof PrinterError) {
      return nativeError;
    }

    // Map common native error messages/codes to PrinterErrorCode
    const message = nativeError?.message || String(nativeError);

    if (message.includes('printer') && message.includes('not found')) {
      return new PrinterError(message, 'PRINTER_NOT_FOUND', nativeError);
    }

    if (message.includes('offline') || message.includes('not available')) {
      return new PrinterError(message, 'PRINTER_OFFLINE', nativeError);
    }

    if (message.includes('access denied') || message.includes('permission')) {
      return new PrinterError(message, 'ACCESS_DENIED', nativeError);
    }

    if (message.includes('job') && message.includes('not found')) {
      return new PrinterError(message, 'JOB_NOT_FOUND', nativeError);
    }

    if (message.includes('driver')) {
      return new PrinterError(message, 'DRIVER_ERROR', nativeError);
    }

    if (message.includes('file') && message.includes('not found')) {
      return new PrinterError(message, 'FILE_NOT_FOUND', nativeError);
    }

    return new PrinterError(message, 'UNKNOWN', nativeError);
  }
}
