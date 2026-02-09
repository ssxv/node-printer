export { printers } from './printers';
export { jobs } from './jobs';
export { PrinterError } from './errors';
import { PrinterError } from './errors';
export type { Printer, PrinterCapabilities, JobStatus, PrintFileOptions, PrintRawOptions, PrintOptions, PrintJobResult, PrinterErrorCode, PrinterDetails, PrintDirectOptions, PrinterDriverOptions } from './types';
/**
 * @deprecated Use printers.list() instead
 */
export declare const getPrinters: any;
/**
 * @deprecated Use printers.get(name) instead
 */
export declare const getPrinter: any;
/**
 * @deprecated Use jobs.printRaw() instead
 */
export declare const printDirect: any;
/**
 * @deprecated Use jobs.printFile() instead
 */
export declare const printFile: any;
declare const _default: {
    printers: {
        list(): Promise<import("./types").Printer[]>;
        default(): Promise<import("./types").Printer>;
        get(name: string): Promise<import("./types").Printer & {
            capabilities?: import("./types").PrinterCapabilities;
        }>;
        capabilities(name: string): Promise<import("./types").PrinterCapabilities>;
        driverOptions(name: string): Promise<import("./types").PrinterDriverOptions>;
    };
    jobs: {
        printFile(options: import("./types").PrintFileOptions): Promise<import("./types").PrintJobResult>;
        printRaw(options: import("./types").PrintRawOptions): Promise<import("./types").PrintJobResult>;
        get(printer: string, jobId: number): Promise<import("./types").JobStatus>;
        list(options?: {
            printer?: string;
        }): Promise<import("./types").JobStatus[]>;
        cancel(printer: string, jobId: number): Promise<void>;
        setNative(printer: string, jobId: number, command: "pause" | "resume" | "cancel"): Promise<void>;
    };
    PrinterError: typeof PrinterError;
    getPrinters: any;
    getPrinter: any;
    printDirect: any;
    printFile: any;
};
export default _default;
//# sourceMappingURL=index.d.ts.map