// Promise-first overloads
export function getPrinters(): Promise<PrinterDetails[]>;
export function getPrinters(cb: (err: Error | null, printers?: PrinterDetails[]) => void): void;

export function getPrinter(printerName?: string): Promise<PrinterDetails>;
export function getPrinter(printerName: string | undefined, cb: (err: Error | null, printer?: PrinterDetails) => void): void;

export function getPrinterDriverOptions(printerName?: string): Promise<PrinterDriverOptions>;
export function getPrinterDriverOptions(printerName: string | undefined, cb: (err: Error | null, opts?: PrinterDriverOptions) => void): void;

export function getSelectedPaperSize(printerName?: string): Promise<string>;
export function getSelectedPaperSize(printerName: string | undefined, cb: (err: Error | null, size?: string) => void): void;

export function getDefaultPrinterName(): Promise<string | undefined>;
export function getDefaultPrinterName(cb: (err: Error | null, name?: string | undefined) => void): void;

export function printDirect(options: PrintDirectOptions): Promise<any>;
export function printDirect(options: PrintDirectOptions, cb: (err: Error | null, res?: any) => void): void;

export function printFile(options: PrintFileOptions): Promise<any>;
export function printFile(options: PrintFileOptions, cb: (err: Error | null, res?: any) => void): void;

export function getSupportedPrintFormats(): Promise<string[]>;
export function getSupportedPrintFormats(cb: (err: Error | null, formats?: string[]) => void): void;

export function getJob(printerName: string, jobId: number): Promise<JobDetails>;
export function getJob(printerName: string, jobId: number, cb: (err: Error | null, job?: JobDetails) => void): void;

export function setJob(printerName: string, jobId: number, command: 'CANCEL' | string): Promise<any>;
export function setJob(printerName: string, jobId: number, command: 'CANCEL' | string, cb: (err: Error | null, res?: any) => void): void;

export function getSupportedJobCommands(): Promise<string[]>;
export function getSupportedJobCommands(cb: (err: Error | null, cmds?: string[]) => void): void;

export interface PrintDirectOptions {
    data: string | Buffer;
    printer?: string | undefined;
    type?: 'RAW' | 'TEXT' | 'PDF' | 'JPEG' | 'POSTSCRIPT' | 'COMMAND' | 'AUTO' | undefined;
    options?: { [key: string]: string } | undefined;
    success?: PrintOnSuccessFunction | undefined;
    error?: PrintOnErrorFunction | undefined;
}

export interface PrintFileOptions {
    filename: string;
    printer?: string | undefined;
    options?: { [key: string]: string } | undefined;
    success?: PrintOnSuccessFunction | undefined;
    error?: PrintOnErrorFunction | undefined;
}

export type PrintOnSuccessFunction = (jobId: string) => any;
export type PrintOnErrorFunction = (err: Error) => any;

export interface PrinterDetails {
    name: string;
    isDefault: boolean;
    options: { [key: string]: string; };
}

export interface PrinterDriverOptions {
    [key: string]: { [key: string]: boolean; };
}

export interface JobDetails {
    id: number;
    name: string;
    printerName: string;
    user: string;
    format: string;
    priority: number;
    size: number;
    status: JobStatus[];
    completedTime: Date;
    creationTime: Date;
    processingTime: Date;
}

export type JobStatus = 'PAUSED' | 'PRINTING' | 'PRINTED' | 'CANCELLED' | 'PENDING' | 'ABORTED';
