"use strict";
// Jobs module - JS-first cross-platform printing API
// Abstracts away OS-specific job management (Winspool/CUPS)
Object.defineProperty(exports, "__esModule", { value: true });
exports.jobs = void 0;
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
 * Normalize job state from OS-specific values to standard states
 */
function normalizeJobState(rawState) {
    if (!rawState)
        return 'error';
    const state = String(rawState).toUpperCase();
    if (state.includes('PRINTING') || state.includes('PROCESSING'))
        return 'printing';
    if (state.includes('COMPLETED') || state.includes('PRINTED'))
        return 'completed';
    if (state.includes('CANCELLED') || state.includes('CANCELED'))
        return 'canceled';
    if (state.includes('PENDING') || state.includes('WAITING'))
        return 'pending';
    if (state.includes('ERROR') || state.includes('ABORTED'))
        return 'error';
    return 'pending'; // Default to pending for unknown states
}
/**
 * Normalize raw job data to standard JobStatus interface
 */
function normalizeJobStatus(raw) {
    return {
        id: raw.id || raw.jobId || 0,
        state: normalizeJobState(raw.status || raw.state),
        printer: raw.printer || raw.printerName || undefined,
        title: raw.title || raw.name || raw.docname || undefined,
        user: raw.user || raw.username || undefined,
        creationTime: raw.creationTime ? new Date(raw.creationTime * 1000) : undefined,
        processingTime: raw.processingTime ? new Date(raw.processingTime * 1000) : undefined,
        completedTime: raw.completedTime ? new Date(raw.completedTime * 1000) : undefined,
        pages: raw.pages || raw.totalPages || undefined,
        size: raw.size || raw.dataSize || undefined
    };
}
/**
 * Validate print options and convert to native format
 */
function validateAndNormalizePrintOptions(options = {}) {
    const normalized = {};
    if (options.copies && options.copies > 0) {
        normalized.copies = Math.floor(options.copies);
    }
    if (typeof options.duplex === 'boolean') {
        normalized.duplex = options.duplex;
    }
    if (typeof options.color === 'boolean') {
        normalized.color = options.color;
    }
    if (options.paperSize && typeof options.paperSize === 'string') {
        normalized.paperSize = options.paperSize;
    }
    if (options.orientation === 'portrait' || options.orientation === 'landscape') {
        normalized.orientation = options.orientation;
    }
    if (options.jobName && typeof options.jobName === 'string') {
        normalized.docname = options.jobName;
    }
    return normalized;
}
exports.jobs = {
    /**
     * Print a file
     */
    async printFile(options) {
        try {
            if (!options.printer || !options.file) {
                throw new errors_1.PrinterError('Printer name and file path are required', 'INVALID_ARGUMENTS');
            }
            const normalizedOptions = validateAndNormalizePrintOptions(options.options);
            const jobId = await binding.printFile({
                filename: options.file,
                printer: options.printer,
                ...normalizedOptions
            });
            if (!jobId || jobId <= 0) {
                throw new errors_1.PrinterError('Failed to queue print job', 'UNKNOWN');
            }
            return { id: jobId, printer: options.printer };
        }
        catch (error) {
            throw errors_1.PrinterError.fromNativeError(error);
        }
    },
    /**
     * Print raw data
     */
    async printRaw(options) {
        try {
            if (!options.printer || !options.data) {
                throw new errors_1.PrinterError('Printer name and data are required', 'INVALID_ARGUMENTS');
            }
            if (!Buffer.isBuffer(options.data)) {
                throw new errors_1.PrinterError('Data must be a Buffer', 'INVALID_ARGUMENTS');
            }
            const normalizedOptions = validateAndNormalizePrintOptions(options.options);
            const jobId = await binding.printDirect({
                data: options.data,
                printer: options.printer,
                type: options.format || 'RAW',
                ...normalizedOptions
            });
            if (!jobId || jobId <= 0) {
                throw new errors_1.PrinterError('Failed to queue print job', 'UNKNOWN');
            }
            return { id: jobId, printer: options.printer };
        }
        catch (error) {
            throw errors_1.PrinterError.fromNativeError(error);
        }
    },
    /**
     * Get status of a specific job
     */
    async get(printer, jobId) {
        try {
            if (!printer || jobId <= 0) {
                throw new errors_1.PrinterError('Valid printer name and job ID are required', 'INVALID_ARGUMENTS');
            }
            const rawJob = await binding.getJob(printer, jobId);
            return normalizeJobStatus(rawJob);
        }
        catch (error) {
            throw errors_1.PrinterError.fromNativeError(error);
        }
    },
    /**
     * List jobs for a specific printer or all printers
     */
    async list(options) {
        try {
            let rawJobs;
            if (options?.printer) {
                // Get jobs for specific printer
                const printer = await binding.getPrinter(options.printer);
                rawJobs = printer.jobs || [];
            }
            else {
                // Get jobs for all printers
                const printers = await binding.getPrinters();
                rawJobs = printers.flatMap((p) => p.jobs || []);
            }
            return rawJobs.map(normalizeJobStatus);
        }
        catch (error) {
            throw errors_1.PrinterError.fromNativeError(error);
        }
    },
    /**
     * Cancel a specific job
     */
    async cancel(printer, jobId) {
        try {
            if (!printer || jobId <= 0) {
                throw new errors_1.PrinterError('Valid printer name and job ID are required', 'INVALID_ARGUMENTS');
            }
            await binding.setJob(printer, jobId, 'CANCEL');
        }
        catch (error) {
            throw errors_1.PrinterError.fromNativeError(error);
        }
    },
    /**
     * Set native job command (escape hatch for platform-specific operations)
     */
    async setNative(printer, jobId, command) {
        try {
            if (!printer || jobId <= 0) {
                throw new errors_1.PrinterError('Valid printer name and job ID are required', 'INVALID_ARGUMENTS');
            }
            const nativeCommand = command.toUpperCase();
            await binding.setJob(printer, jobId, nativeCommand);
        }
        catch (error) {
            throw errors_1.PrinterError.fromNativeError(error);
        }
    }
};
//# sourceMappingURL=jobs.js.map