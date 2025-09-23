// High-level wrapper for node-printer-new
// Provides backward-compatible API (callbacks) and Promise-based usage.

const binding = require("./index");

function toDateOrNull(n) {
  if (n === 0 || n == null) return null;
  return new Date(n * 1000);
}

function normalizeJob(job) {
  return Object.assign({}, job, {
    creationTime: toDateOrNull(job.creationTime),
    processingTime: toDateOrNull(job.processingTime),
    completedTime: toDateOrNull(job.completedTime),
  });
}

// Expose internals for unit testing (non-production API)
module.exports.__test__ = {
  toDateOrNull,
  normalizeJob,
};

function normalizePrinter(p) {
  const copy = Object.assign({}, p);
  if (Array.isArray(copy.jobs)) copy.jobs = copy.jobs.map(normalizeJob);
  return copy;
}

function promiseify(fn) {
  return function (...args) {
    const maybeCb =
      typeof args[args.length - 1] === "function" ? args.pop() : null;
    try {
      const res = fn.apply(this, args);
      const promise =
        res && typeof res.then === "function" ? res : Promise.resolve(res);
      if (maybeCb) {
        promise
          .then((r) => process.nextTick(() => maybeCb(null, r)))
          .catch((e) => process.nextTick(() => maybeCb(e)));
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

module.exports.getPrinters = promiseify(function getPrinters() {
  const result = binding.getPrinters();
  return Array.isArray(result) ? result.map(normalizePrinter) : result;
});

module.exports.getDefaultPrinterName = promiseify(
  function getDefaultPrinterName() {
    const name =
      binding.getDefaultPrinterName && binding.getDefaultPrinterName();
    if (name) return name;

    // Fallback: search printers for isDefault flag
    const printers = (binding.getPrinters && binding.getPrinters()) || [];
    if (Array.isArray(printers)) {
      for (let i = 0; i < printers.length; i++) {
        if (printers[i] && printers[i].isDefault === true)
          return printers[i].name;
      }
    }
    // undefined if not found
    return undefined;
  }
);

module.exports.getPrinter = promiseify(function getPrinter(name) {
  // allow name to be optional
  if (!name)
    name =
      (binding.getDefaultPrinterName && binding.getDefaultPrinterName()) ||
      undefined;
  const pr = binding.getPrinter(name);
  return pr && Array.isArray(pr.jobs) ? normalizePrinter(pr) : pr;
});

module.exports.getPrinterDriverOptions = promiseify(
  function getPrinterDriverOptions(name) {
    if (!name)
      name =
        (binding.getDefaultPrinterName && binding.getDefaultPrinterName()) ||
        undefined;
    return (
      binding.getPrinterDriverOptions && binding.getPrinterDriverOptions(name)
    );
  }
);

module.exports.getSelectedPaperSize = promiseify(function getSelectedPaperSize(
  name
) {
  const driver_options =
    (binding.getPrinterDriverOptions &&
      binding.getPrinterDriverOptions(name)) ||
    {};
  let selectedSize = "";
  if (driver_options && driver_options.PageSize) {
    Object.keys(driver_options.PageSize).forEach(function (key) {
      if (driver_options.PageSize[key]) selectedSize = key;
    });
  }
  return selectedSize;
});

module.exports.getJob = promiseify(function getJob(printerName, jobId) {
  return binding.getJob && binding.getJob(printerName, jobId);
});

module.exports.setJob = promiseify(function setJob(
  printerName,
  jobId,
  command
) {
  return binding.setJob && binding.setJob(printerName, jobId, command);
});

function wirePromiseOrSync(result, success, error) {
  if (result && typeof result.then === "function") {
    result
      .then(function (r) {
        if (typeof success === "function") success(r);
      })
      .catch(function (e) {
        if (typeof error === "function") error(e);
        else throw e;
      });
    return true;
  }
  // synchronous
  if (typeof success === "function") success(result);
  return false;
}

module.exports.printDirect = function printDirect(
  parameters /* or (data, printer, type, docname, options, callback) */
) {
  // Handle both object and legacy parameter forms
  if (
    typeof parameters === "object" &&
    !Buffer.isBuffer(parameters) &&
    parameters !== null &&
    arguments.length <= 2
  ) {
    // Object form: printDirect({ data, printer, docname, type, options, success, error }) or printDirect(options, callback)
    const { data, printer, docname, type, options, success, error } =
      parameters;
    const nodeCallback =
      arguments.length === 2 && typeof arguments[1] === "function"
        ? arguments[1]
        : null;

    // Create inner function that can be promiseified
    const printDirectCore = function (data, printer, docname, type, options) {
      if (!binding.printDirect) {
        throw new Error("Not supported");
      }

      const normalizedType = (type || "RAW").toString().toUpperCase();
      const finalPrinter = printer || module.exports.getDefaultPrinterName();
      const finalDocname = docname || "node print job";

      return binding.printDirect(
        data,
        finalPrinter,
        finalDocname,
        normalizedType,
        options || {}
      );
    };

    // If there's a Node.js-style callback, use promiseify
    if (nodeCallback) {
      const wrappedFn = promiseify(printDirectCore);
      return wrappedFn(data, printer, docname, type, options, nodeCallback);
    }

    // Handle legacy success/error callbacks if present - but still return Promise
    const wrappedFn = promiseify(printDirectCore);
    const promise = wrappedFn(data, printer, docname, type, options);

    if (typeof success === "function" || typeof error === "function") {
      promise
        .then((r) => {
          if (typeof success === "function") success(r);
        })
        .catch((e) => {
          if (typeof error === "function") error(e);
        });
    }

    return promise;
  } else {
    // Legacy parameter form: printDirect(data, printer, type, docname, options, callback)
    const printDirectCore = function (data, printer, type, docname, options) {
      if (!binding.printDirect) {
        throw new Error("Not supported");
      }

      const normalizedType = (type || "RAW").toString().toUpperCase();
      const finalPrinter = printer || module.exports.getDefaultPrinterName();
      const finalDocname = docname || "node print job";

      return binding.printDirect(
        data,
        finalPrinter,
        finalDocname,
        normalizedType,
        options || {}
      );
    };

    return promiseify(printDirectCore)(...arguments);
  }
};

module.exports.printFile = function printFile(parameters, cb) {
  // Accept both: printFile(params) and printFile(params, cb)
  if (arguments.length < 1 || typeof parameters !== "object") {
    throw new Error("must provide arguments object");
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
      throw new Error("must provide at least a filename");
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
      finalPrinter = module.exports.getDefaultPrinterName();
    }

    if (!finalPrinter) {
      throw new Error("Printer parameter or default printer is not defined");
    }

    if (!binding.printFile) {
      throw new Error("Not supported");
    }

    return binding.printFile(filename, finalDocname, finalPrinter, options);
  };

  // Use promiseify pattern consistently
  const wrappedFn = promiseify(printFileCore);
  const promise = wrappedFn(filename, docname, printer, options, cb);

  // Handle legacy success/error callbacks if present (but no Node.js callback)
  if (
    (typeof legacySuccess === "function" ||
      typeof legacyError === "function") &&
    typeof cb !== "function"
  ) {
    promise
      .then((r) => {
        if (typeof legacySuccess === "function") legacySuccess(r);
      })
      .catch((e) => {
        if (typeof legacyError === "function") legacyError(e);
      });
  }

  return promise;
};

module.exports.getSupportedPrintFormats = promiseify(
  function getSupportedPrintFormats() {
    return (
      binding.getSupportedPrintFormats && binding.getSupportedPrintFormats()
    );
  }
);

module.exports.getSupportedJobCommands = promiseify(
  function getSupportedJobCommands() {
    return binding.getSupportedJobCommands && binding.getSupportedJobCommands();
  }
);
