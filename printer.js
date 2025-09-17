// High-level wrapper for node-printer-new
// Provides backward-compatible API (callbacks) and Promise-based usage.

const binding = require('./index');

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

function normalizePrinter(p) {
  const copy = Object.assign({}, p);
  if (Array.isArray(copy.jobs)) copy.jobs = copy.jobs.map(normalizeJob);
  return copy;
}

function promiseify(fn) {
  return function(...args) {
    const maybeCb = typeof args[args.length - 1] === 'function' ? args.pop() : null;
    try {
      const res = fn.apply(this, args);
      const promise = (res && typeof res.then === 'function') ? res : Promise.resolve(res);
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

module.exports.getPrinters = promiseify(function getPrinters() {
  const result = binding.getPrinters();
  return Array.isArray(result) ? result.map(normalizePrinter) : result;
});

module.exports.getDefaultPrinterName = promiseify(function getDefaultPrinterName() {
  const name = binding.getDefaultPrinterName && binding.getDefaultPrinterName();
  if (name) return name;

  // Fallback: search printers for isDefault flag
  const printers = (binding.getPrinters && binding.getPrinters()) || [];
  if (Array.isArray(printers)) {
    for (let i = 0; i < printers.length; i++) {
      if (printers[i] && printers[i].isDefault === true) return printers[i].name;
    }
  }
  // undefined if not found
  return undefined;
});

module.exports.getPrinter = promiseify(function getPrinter(name) {
  // allow name to be optional
  if (!name) name = (binding.getDefaultPrinterName && binding.getDefaultPrinterName()) || undefined;
  const pr = binding.getPrinter(name);
  return pr && Array.isArray(pr.jobs) ? normalizePrinter(pr) : pr;
});

module.exports.getPrinterDriverOptions = promiseify(function getPrinterDriverOptions(name) {
  if (!name) name = (binding.getDefaultPrinterName && binding.getDefaultPrinterName()) || undefined;
  return binding.getPrinterDriverOptions && binding.getPrinterDriverOptions(name);
});

module.exports.getSelectedPaperSize = promiseify(function getSelectedPaperSize(name) {
  const driver_options = (binding.getPrinterDriverOptions && binding.getPrinterDriverOptions(name)) || {};
  let selectedSize = "";
  if (driver_options && driver_options.PageSize) {
    Object.keys(driver_options.PageSize).forEach(function(key) {
      if (driver_options.PageSize[key]) selectedSize = key;
    });
  }
  return selectedSize;
});

module.exports.getJob = promiseify(function getJob(printerName, jobId) {
  return binding.getJob && binding.getJob(printerName, jobId);
});

module.exports.setJob = promiseify(function setJob(printerName, jobId, command) {
  return binding.setJob && binding.setJob(printerName, jobId, command);
});

function wirePromiseOrSync(result, success, error) {
  if (result && typeof result.then === 'function') {
    result.then(function(r) {
      if (typeof success === 'function') success(r);
    }).catch(function(e) {
      if (typeof error === 'function') error(e);
      else throw e;
    });
    return true;
  }
  // synchronous
  if (typeof success === 'function') success(result);
  return false;
}

module.exports.printDirect = function printDirect(parameters /* or (data, printer, type, docname, options, success, error) */) {
  let data, printer, docname, type, options, success, error;

  if (arguments.length === 1 && typeof parameters === 'object') {
    data = parameters.data;
    printer = parameters.printer;
    docname = parameters.docname;
    type = parameters.type;
    options = parameters.options || {};
    success = parameters.success;
    error = parameters.error;
  } else {
    data = arguments[0];
    printer = arguments[1];
    type = arguments[2];
    docname = arguments[3];
    options = arguments[4] || {};
    success = arguments[5];
    error = arguments[6];
  }

  if (!type) type = 'RAW';
  if (!docname) docname = 'node print job';
  if (!printer) printer = module.exports.getDefaultPrinterName();

  const params = { data, printer, docname, type: (type || '').toString().toUpperCase(), options };

  if (binding.printDirect) {
    try {
      const res = binding.printDirect(params);
      // wire callbacks or return
      if (typeof success === 'function' || typeof error === 'function') {
        if (res && typeof res.then === 'function') {
          res.then(function(r) {
            if (r) {
              if (typeof success === 'function') success(r);
            } else {
              if (typeof error === 'function') error(new Error('Something wrong in printDirect'));
            }
          }).catch(function(e) {
            if (typeof error === 'function') error(e);
            else throw e;
          });
          return res;
        }
        // synchronous/native
        if (res) {
          if (typeof success === 'function') success(res);
          return Promise.resolve(res);
        } else {
          if (typeof error === 'function') error(new Error('Something wrong in printDirect'));
          else throw new Error('Something wrong in printDirect');
        }
      }

      // No callbacks: return Promise if native returned one, else Promise.resolve
      if (res && typeof res.then === 'function') return res;
      return Promise.resolve(res);
    } catch (e) {
      if (typeof error === 'function') error(e);
      else throw e;
    }
  } else {
    if (typeof error === 'function') error('Not supported');
    else throw new Error('Not supported');
  }
};

module.exports.printFile = function printFile(parameters, cb) {
  // Accept both: printFile(params) and printFile(params, cb)
  if ((arguments.length < 1) || (typeof parameters !== 'object')) {
    throw new Error('must provide arguments object');
  }

  const filename = parameters.filename || parameters;
  let docname = parameters.docname;
  let printer = parameters.printer;
  const options = parameters.options || {};
  const userCallback = typeof cb === 'function' ? cb : (typeof parameters.success === 'function' ? parameters.success : null);
  const userError = typeof parameters.error === 'function' ? parameters.error : null;

  if (!filename) {
    const err = new Error('must provide at least a filename');
    if (userError) return userError(err);
    throw err;
  }

  if (!docname) docname = filename;

  // If no printer provided, prefer calling binding.printFile(filename) when available
  if (!printer) {
    // Try to call binding.printFile with filename only (some native bindings expect this)
    if (binding.printFile && binding.printFile.length === 1) {
      try {
        const res = binding.printFile(filename);
        if (userCallback) {
          if (res && typeof res.then === 'function') {
            res.then(r => userCallback(null, r)).catch(err => userCallback(err));
            return res;
          }
          process.nextTick(() => userCallback(null, res));
          return Promise.resolve(res);
        }
        if (res && typeof res.then === 'function') return res;
        return Promise.resolve(res);
      } catch (e) {
        if (userError) return userError(e);
        throw e;
      }
    }

    // else, try to resolve default printer name
    printer = module.exports.getDefaultPrinterName();
  }

  if (!printer) {
    const err = new Error('Printer parameter or default printer is not defined');
    if (userError) return userError(err);
    throw err;
  }

  // now call binding.printFile with expanded args if available
  if (binding.printFile) {
    try {
      const res = binding.printFile(filename, docname, printer, options);
      if (userCallback) {
        if (res && typeof res.then === 'function') {
          res.then(function(r) {
            userCallback(null, r);
          }).catch(function(e) {
            userCallback(e);
          });
          return res;
        }
        process.nextTick(() => userCallback(null, res));
        return Promise.resolve(res);
      }
      if (res && typeof res.then === 'function') return res;
      return Promise.resolve(res);
    } catch (e) {
      if (userError) return userError(e);
      throw e;
    }
  } else {
    if (userError) return userError('Not supported');
    throw new Error('Not supported');
  }
};

module.exports.getSupportedPrintFormats = promiseify(function getSupportedPrintFormats() {
  return binding.getSupportedPrintFormats && binding.getSupportedPrintFormats();
});

module.exports.getSupportedJobCommands = promiseify(function getSupportedJobCommands() {
  return binding.getSupportedJobCommands && binding.getSupportedJobCommands();
});
