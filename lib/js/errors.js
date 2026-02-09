"use strict";
// Error types for @ssxv/node-printer
// Cross-platform error normalization
Object.defineProperty(exports, "__esModule", { value: true });
exports.PrinterError = void 0;
class PrinterError extends Error {
    constructor(message, code = 'UNKNOWN', originalError) {
        super(message);
        this.name = 'PrinterError';
        this.code = code;
        this.originalError = originalError;
        // Maintains proper stack trace for where error was thrown (only on V8)
        if (Error.captureStackTrace) {
            Error.captureStackTrace(this, PrinterError);
        }
    }
    static fromNativeError(nativeError) {
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
exports.PrinterError = PrinterError;
//# sourceMappingURL=errors.js.map