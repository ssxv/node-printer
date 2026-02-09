export type PrinterErrorCode = 'PRINTER_NOT_FOUND' | 'PRINTER_OFFLINE' | 'ACCESS_DENIED' | 'JOB_NOT_FOUND' | 'DRIVER_ERROR' | 'INVALID_ARGUMENTS' | 'FILE_NOT_FOUND' | 'UNSUPPORTED_FORMAT' | 'UNKNOWN';
export declare class PrinterError extends Error {
    readonly code: PrinterErrorCode;
    readonly originalError?: any;
    constructor(message: string, code?: PrinterErrorCode, originalError?: any);
    static fromNativeError(nativeError: any): PrinterError;
}
//# sourceMappingURL=errors.d.ts.map