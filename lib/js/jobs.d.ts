import { JobStatus, PrintFileOptions, PrintRawOptions, PrintJobResult } from './types';
export declare const jobs: {
    /**
     * Print a file
     */
    printFile(options: PrintFileOptions): Promise<PrintJobResult>;
    /**
     * Print raw data
     */
    printRaw(options: PrintRawOptions): Promise<PrintJobResult>;
    /**
     * Get status of a specific job
     */
    get(printer: string, jobId: number): Promise<JobStatus>;
    /**
     * List jobs for a specific printer or all printers
     */
    list(options?: {
        printer?: string;
    }): Promise<JobStatus[]>;
    /**
     * Cancel a specific job
     */
    cancel(printer: string, jobId: number): Promise<void>;
    /**
     * Set native job command (escape hatch for platform-specific operations)
     */
    setNative(printer: string, jobId: number, command: "pause" | "resume" | "cancel"): Promise<void>;
};
//# sourceMappingURL=jobs.d.ts.map