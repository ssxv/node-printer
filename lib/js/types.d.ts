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
export interface JobStatus {
    id: number;
    state: 'pending' | 'printing' | 'completed' | 'canceled' | 'error';
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
export type PrinterErrorCode = 'PRINTER_NOT_FOUND' | 'PRINTER_OFFLINE' | 'ACCESS_DENIED' | 'JOB_NOT_FOUND' | 'DRIVER_ERROR' | 'INVALID_ARGUMENTS' | 'FILE_NOT_FOUND' | 'UNSUPPORTED_FORMAT' | 'UNKNOWN';
export interface PrinterDetails extends Printer {
    status?: string[];
    jobs?: JobStatus[];
    options?: Record<string, any>;
}
export interface PrintDirectOptions {
    data: string | Buffer;
    printer: string;
    type?: string;
    docname?: string;
    options?: Record<string, string>;
}
export interface PrinterDriverOptions {
    [key: string]: any;
}
//# sourceMappingURL=types.d.ts.map