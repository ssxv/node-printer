"use strict";
// Printers module - JS-first cross-platform API
// Abstracts away OS-specific concepts (Winspool/CUPS)
Object.defineProperty(exports, "__esModule", { value: true });
exports.printers = void 0;
const errors_1 = require("./errors");
// Dynamic binding loader - will be replaced with proper loader
let binding;
try {
    // For now, use the existing binding until we refactor the native layer
    binding = require('../../binding');
}
catch (error) {
    throw new errors_1.PrinterError('Failed to load native printer binding', 'DRIVER_ERROR', error);
}
/**
 * Normalize printer state from OS-specific values to standard states
 */
function normalizePrinterState(rawState) {
    if (!rawState)
        return 'offline';
    // Handle Windows status arrays
    if (Array.isArray(rawState)) {
        if (rawState.includes('PRINTING') || rawState.includes('PROCESSING'))
            return 'printing';
        if (rawState.includes('PAUSED') || rawState.includes('STOPPED'))
            return 'stopped';
        if (rawState.includes('ERROR') || rawState.includes('PAPER-JAM') || rawState.includes('NO-TONER'))
            return 'error';
        if (rawState.includes('OFFLINE') || rawState.includes('NOT-AVAILABLE'))
            return 'offline';
        return 'idle';
    }
    // Handle string status
    const status = String(rawState).toUpperCase();
    if (status.includes('PRINTING') || status.includes('PROCESSING'))
        return 'printing';
    if (status.includes('PAUSED') || status.includes('STOPPED'))
        return 'stopped';
    if (status.includes('ERROR') || status.includes('JAM') || status.includes('TONER'))
        return 'error';
    if (status.includes('OFFLINE') || status.includes('AVAILABLE'))
        return 'offline';
    return 'idle';
}
/**
 * Normalize raw printer data to standard Printer interface
 */
function normalizePrinter(raw) {
    return {
        name: raw.name || '',
        isDefault: Boolean(raw.isDefault),
        state: normalizePrinterState(raw.status),
        location: raw.location || undefined,
        description: raw.description || raw.comment || undefined
    };
}
/**
 * Normalize capabilities from raw driver data
 */
function normalizeCapabilities(raw) {
    const formats = ['RAW']; // Always support RAW
    // Add other formats based on raw data
    if (raw.pdf || raw.PDF)
        formats.push('PDF');
    if (raw.text || raw.TEXT)
        formats.push('TEXT');
    if (raw.image || raw.IMAGE || raw.jpeg || raw.jpg)
        formats.push('IMAGE');
    return {
        formats,
        paperSizes: raw.paperSizes || undefined,
        duplex: raw.duplex === true || raw.duplex === 'supported',
        color: raw.color === true || raw.color === 'supported'
    };
}
exports.printers = {
    /**
     * List all available printers
     */
    async list() {
        try {
            const rawPrinters = await binding.getPrinters();
            return rawPrinters.map(normalizePrinter);
        }
        catch (error) {
            throw errors_1.PrinterError.fromNativeError(error);
        }
    },
    /**
     * Get the default printer
     */
    async default() {
        try {
            const defaultName = await binding.getDefaultPrinterName();
            if (!defaultName) {
                throw new errors_1.PrinterError('No default printer found', 'PRINTER_NOT_FOUND');
            }
            return this.get(defaultName);
        }
        catch (error) {
            throw errors_1.PrinterError.fromNativeError(error);
        }
    },
    /**
     * Get detailed information about a specific printer
     */
    async get(name) {
        try {
            const rawPrinter = await binding.getPrinter(name);
            const printer = normalizePrinter(rawPrinter);
            // Try to get capabilities - non-critical, so don't fail if unavailable
            let capabilities;
            try {
                capabilities = await this.capabilities(name);
            }
            catch {
                // Capabilities optional - continue without them
            }
            return { ...printer, capabilities };
        }
        catch (error) {
            throw errors_1.PrinterError.fromNativeError(error);
        }
    },
    /**
     * Get printer capabilities (formats, paper sizes, etc.)
     */
    async capabilities(name) {
        try {
            // Get supported formats
            const formats = await binding.getSupportedPrintFormats();
            // Get driver options for additional capabilities
            let driverOptions = {};
            try {
                driverOptions = await binding.getPrinterDriverOptions(name);
            }
            catch {
                // Driver options might not be available on all platforms
            }
            return normalizeCapabilities({ ...driverOptions, formats });
        }
        catch (error) {
            throw errors_1.PrinterError.fromNativeError(error);
        }
    },
    /**
     * Get raw driver options (advanced/unstable API)
     * Exposes platform-specific data without normalization
     */
    async driverOptions(name) {
        try {
            return await binding.getPrinterDriverOptions(name);
        }
        catch (error) {
            throw errors_1.PrinterError.fromNativeError(error);
        }
    }
};
//# sourceMappingURL=printers.js.map