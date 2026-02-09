import { Printer, PrinterCapabilities, PrinterDriverOptions } from './types';
export declare const printers: {
    /**
     * List all available printers
     */
    list(): Promise<Printer[]>;
    /**
     * Get the default printer
     */
    default(): Promise<Printer>;
    /**
     * Get detailed information about a specific printer
     */
    get(name: string): Promise<Printer & {
        capabilities?: PrinterCapabilities;
    }>;
    /**
     * Get printer capabilities (formats, paper sizes, etc.)
     */
    capabilities(name: string): Promise<PrinterCapabilities>;
    /**
     * Get raw driver options (advanced/unstable API)
     * Exposes platform-specific data without normalization
     */
    driverOptions(name: string): Promise<PrinterDriverOptions>;
};
//# sourceMappingURL=printers.d.ts.map