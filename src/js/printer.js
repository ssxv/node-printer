// Simplified legacy API wrapper
// Provides only core printing functions with backward-compatible API (callbacks) and Promise-based usage.
// For full functionality, use the modern JS-first API: require('@ssxv/node-printer')

const binding = require('../../binding-legacy');

function toDateOrNull(n) {
  if (n === 0 || n == null) return null;
  return new Date(n * 1000);
}

function normalizeJob(job) {
  return Object.assign({}, job, {
    creationTime: toDateOrNull(job.creationTime),
    processingTime: toDateOrNull(job.processingTime),
    completedTime: toDateOrNull(job.completedTime)
  });
}

// Expose internals for unit testing (non-production API)
module.exports.__test__ = {
  toDateOrNull,
  normalizeJob
};

function normalizePrinter(p) {
  const copy = Object.assign({}, p);
  if (Array.isArray(copy.jobs)) copy.jobs = copy.jobs.map(normalizeJob);
  return copy;
}

function promiseify(fn) {
  return function (...args) {
    const maybeCb = typeof args[args.length - 1] === 'function' ? args.pop() : null;
    try {
      const res = fn.apply(this, args);
      const promise = res && typeof res.then === 'function' ? res : Promise.resolve(res);
      if (maybeCb) {
        promise.then(r => process.nextTick(() => maybeCb(null, r))).catch(e => process.nextTick(() => maybeCb(e)));
        return promise;
      }
      return promise;
    } catch (e) {
      if (maybeCb) {
        process.nextTick(() => maybeCb(e));
        return Promise.reject(e);
      }
      return Promise.reject(e);
    }
  };
}

/**
 * Get list of all available printers
 * @deprecated Use require('@ssxv/node-printer').printers.list() instead
 * @param {function} [callback] - Optional Node.js style callback
 * @returns {Promise<Array>} Promise resolving to array of printer objects
 */
module.exports.getPrinters = promiseify(function getPrinters() {
  const result = binding.getPrinters();
  return Array.isArray(result) ? result.map(normalizePrinter) : result;
});

/**
 * Get detailed information about a specific printer
 * @deprecated Use require('@ssxv/node-printer').printers.get(name) instead
 * @param {string} [name] - Printer name (uses default if not specified)
 * @param {function} [callback] - Optional Node.js style callback
 * @returns {Promise<Object>} Promise resolving to printer details object
 */
module.exports.getPrinter = promiseify(function getPrinter(name) {
  // allow name to be optional - use default printer if not specified
  if (!name) {
    const defaultName = binding.getDefaultPrinterName && binding.getDefaultPrinterName();
    if (!defaultName) {
      throw new Error('No printer name provided and no default printer available');
    }
    name = defaultName;
  }
  const pr = binding.getPrinter(name);
  return pr && Array.isArray(pr.jobs) ? normalizePrinter(pr) : pr;
});

/**
 * Print raw data directly to printer
 * @deprecated Use require('@ssxv/node-printer').jobs.printRaw() instead
 * @param {Object|Buffer} parameters - Print parameters object or raw data
 * @param {function} [callback] - Optional Node.js style callback
 * @returns {Promise<Object>} Promise resolving to print job result
 */
module.exports.printDirect = function printDirect(
  parameters /* or (data, printer, type, docname, options, callback) */
) {
  // Handle both object and legacy parameter forms
  if (typeof parameters === 'object' && !Buffer.isBuffer(parameters) && parameters !== null && arguments.length <= 2) {
    // Object form: printDirect({ data, printer, docname, type, options, success, error }) or printDirect(options, callback)
    const { data, printer, docname, type, options, success, error } = parameters;
    const nodeCallback = arguments.length === 2 && typeof arguments[1] === 'function' ? arguments[1] : null;

    // Create inner function that can be promiseified
    const printDirectCore = function (data, printer, docname, type, options) {
      if (!binding.printDirect) {
        throw new Error('Not supported');
      }

      const normalizedType = (type || 'RAW').toString().toUpperCase();
      let finalPrinter = printer;
      if (!finalPrinter) {
        const defaultName = binding.getDefaultPrinterName && binding.getDefaultPrinterName();
        if (!defaultName) {
          throw new Error('No printer specified and no default printer available');
        }
        finalPrinter = defaultName;
      }
      const finalDocname = docname || 'node print job';

      return binding.printDirect(data, finalPrinter, finalDocname, normalizedType, options || {});
    };

    // If there's a Node.js-style callback, use promiseify
    if (nodeCallback) {
      const wrappedFn = promiseify(printDirectCore);
      return wrappedFn(data, printer, docname, type, options, nodeCallback);
    }

    // Handle legacy success/error callbacks if present - but still return Promise
    const wrappedFn = promiseify(printDirectCore);
    const promise = wrappedFn(data, printer, docname, type, options);

    if (typeof success === 'function' || typeof error === 'function') {
      promise
        .then(r => {
          if (typeof success === 'function') success(r);
        })
        .catch(e => {
          if (typeof error === 'function') error(e);
        });
    }

    return promise;
  } else {
    // Legacy parameter form: printDirect(data, printer, type, docname, options, callback)
    const printDirectCore = function (data, printer, type, docname, options) {
      if (!binding.printDirect) {
        throw new Error('Not supported');
      }

      const normalizedType = (type || 'RAW').toString().toUpperCase();
      let finalPrinter = printer;
      if (!finalPrinter) {
        const defaultName = binding.getDefaultPrinterName && binding.getDefaultPrinterName();
        if (!defaultName) {
          throw new Error('No printer specified and no default printer available');
        }
        finalPrinter = defaultName;
      }
      const finalDocname = docname || 'node print job';

      return binding.printDirect(data, finalPrinter, finalDocname, normalizedType, options || {});
    };

    return promiseify(printDirectCore)(...arguments);
  }
};

/**
 * Print a file to the specified printer
 * @deprecated Use require('@ssxv/node-printer').jobs.printFile() instead
 * @param {Object} parameters - Print parameters object with filename, printer, etc.
 * @param {function} [cb] - Optional Node.js style callback
 * @returns {Promise<Object>} Promise resolving to print job result
 */
module.exports.printFile = function printFile(parameters, cb) {
  // Accept both: printFile(params) and printFile(params, cb)
  if (arguments.length < 1 || typeof parameters !== 'object') {
    throw new Error('must provide arguments object');
  }

  const filename = parameters.filename || parameters;
  const docname = parameters.docname;
  const printer = parameters.printer;
  const options = parameters.options || {};
  const legacySuccess = parameters.success;
  const legacyError = parameters.error;

  // Create core function that can be promiseified
  const printFileCore = function (filename, docname, printer, options) {
    if (!filename) {
      throw new Error('must provide at least a filename');
    }

    const finalDocname = docname || filename;
    let finalPrinter = printer;

    // If no printer provided, prefer calling binding.printFile(filename) when available
    if (!finalPrinter) {
      // Try to call binding.printFile with filename only (some native bindings expect this)
      if (binding.printFile && binding.printFile.length === 1) {
        return binding.printFile(filename);
      }

      // else, try to resolve default printer name
      const defaultName = binding.getDefaultPrinterName && binding.getDefaultPrinterName();
      if (!defaultName) {
        throw new Error('No printer specified and no default printer available');
      }
      finalPrinter = defaultName;
    }

    if (!binding.printFile) {
      throw new Error('Not supported');
    }

    return binding.printFile(filename, finalDocname, finalPrinter, options);
  };

  // Use promiseify pattern consistently
  const wrappedFn = promiseify(printFileCore);
  const promise = wrappedFn(filename, docname, printer, options, cb);

  // Handle legacy success/error callbacks if present (but no Node.js callback)
  if ((typeof legacySuccess === 'function' || typeof legacyError === 'function') && typeof cb !== 'function') {
    promise
      .then(r => {
        if (typeof legacySuccess === 'function') legacySuccess(r);
      })
      .catch(e => {
        if (typeof legacyError === 'function') legacyError(e);
      });
  }

  return promise;
};
