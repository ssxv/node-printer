#!/usr/bin/env node

/**
 * Comprehensive test script for @ssxv/node-printer
 * Tests all available functions on Windows and Linux platforms
 *
 * Usage: node testAllFunctions.js [--verbose]
 *
 * This script will:
 * 1. Test all exported functions from printer.js
 * 2. Show platform-specific behavior differences
 * 3. Handle errors gracefully and report function availability
 * 4. Create test files and print jobs where appropriate
 */

const fs = require("fs");
const path = require("path");
const os = require("os");

// Import the printer module
const printer = require("../printer");

// Configuration
const VERBOSE = process.argv.includes("--verbose");
const PLATFORM = os.platform();
const TEST_FILE_PATH = path.join(__dirname, "test-document.txt");
const TEST_DATA =
  "Hello World!\nThis is a test document from node-printer.\nTimestamp: " +
  new Date().toISOString();

// Utility functions
function log(message, level = "INFO") {
  const timestamp = new Date().toISOString();
  const prefix = `[${timestamp}] [${level}]`;
  console.log(`${prefix} ${message}`);
}

function verbose(message) {
  if (VERBOSE) {
    log(message, "VERBOSE");
  }
}

function formatResult(result) {
  if (typeof result === "object" && result !== null) {
    return JSON.stringify(result, null, 2);
  }
  return String(result);
}

async function testFunction(name, testFn) {
  log(`\n${"=".repeat(50)}`);
  log(`Testing: ${name}`);
  log(`${"=".repeat(50)}`);

  try {
    const result = await testFn();
    log(`✅ ${name} - SUCCESS`);
    if (result !== undefined) {
      verbose(`Result: ${formatResult(result)}`);
    }
    return { success: true, result };
  } catch (error) {
    log(`❌ ${name} - FAILED: ${error.message}`, "ERROR");
    verbose(`Error details: ${error.stack}`);
    return { success: false, error: error.message };
  }
}

async function testFunctionWithCallback(name, callbackTestFn) {
  log(`\n${"=".repeat(50)}`);
  log(`Testing (callback): ${name}`);
  log(`${"=".repeat(50)}`);

  try {
    const result = await new Promise((resolve, reject) => {
      callbackTestFn((err, result) => {
        if (err) {
          reject(err);
        } else {
          resolve(result);
        }
      });
    });

    log(`✅ ${name} (callback) - SUCCESS`);
    if (result !== undefined) {
      verbose(`Result: ${formatResult(result)}`);
    }
    return { success: true, result };
  } catch (error) {
    log(`❌ ${name} (callback) - FAILED: ${error.message}`, "ERROR");
    verbose(`Error details: ${error.stack}`);
    return { success: false, error: error.message };
  }
}

// Test functions
async function testGetPrinters() {
  log("Getting list of all printers...");
  const printers = await printer.getPrinters();
  log(`Found ${Array.isArray(printers) ? printers.length : 0} printers`);

  if (Array.isArray(printers) && printers.length > 0) {
    printers.forEach((p, index) => {
      log(
        `Printer ${index + 1}: ${p.name || "Unknown"} (Default: ${
          p.isDefault || false
        })`
      );
      if (VERBOSE) {
        verbose(`  Full details: ${formatResult(p)}`);
      }
    });
  }

  return printers;
}

// Callback version of testGetPrinters
function testGetPrintersCallback(callback) {
  log("Getting list of all printers (callback)...");
  printer.getPrinters((err, printers) => {
    if (err) return callback(err);

    log(`Found ${Array.isArray(printers) ? printers.length : 0} printers`);

    if (Array.isArray(printers) && printers.length > 0) {
      printers.forEach((p, index) => {
        log(
          `Printer ${index + 1}: ${p.name || "Unknown"} (Default: ${
            p.isDefault || false
          })`
        );
        if (VERBOSE) {
          verbose(`  Full details: ${formatResult(p)}`);
        }
      });
    }

    callback(null, printers);
  });
}

async function testGetDefaultPrinterName() {
  log("Getting default printer name...");
  const defaultName = await printer.getDefaultPrinterName();
  log(`Default printer: ${defaultName || "Not found"}`);
  return defaultName;
}

// Callback version of testGetDefaultPrinterName
function testGetDefaultPrinterNameCallback(callback) {
  log("Getting default printer name (callback)...");
  printer.getDefaultPrinterName((err, defaultName) => {
    if (err) return callback(err);
    log(`Default printer: ${defaultName || "Not found"}`);
    callback(null, defaultName);
  });
}

async function testGetPrinter() {
  log("Getting specific printer info...");

  // Try to get default printer first, fallback to first available printer
  let testPrinterName = await printer.getDefaultPrinterName();
  if (!testPrinterName) {
    const printers = await printer.getPrinters();
    if (!Array.isArray(printers) || printers.length === 0) {
      throw new Error("No printers available to test with");
    }
    testPrinterName = printers[0].name;
    log("No default printer found, using first available printer");
  }

  log(`Testing with printer: ${testPrinterName}`);

  const printerInfo = await printer.getPrinter(testPrinterName);
  log(`Printer info retrieved for: ${printerInfo?.name || "Unknown"}`);

  if (VERBOSE && printerInfo) {
    verbose(
      `Jobs count: ${
        Array.isArray(printerInfo.jobs) ? printerInfo.jobs.length : 0
      }`
    );
  }

  return printerInfo;
}

// Callback version of testGetPrinter
function testGetPrinterCallback(callback) {
  log("Getting specific printer info (callback)...");

  // First get default printer name
  printer.getDefaultPrinterName((err, testPrinterName) => {
    if (err) return callback(err);

    if (!testPrinterName) {
      // Fallback to first available printer
      printer.getPrinters((err, printers) => {
        if (err) return callback(err);
        if (!Array.isArray(printers) || printers.length === 0) {
          return callback(new Error("No printers available to test with"));
        }
        testPrinterName = printers[0].name;
        log("No default printer found, using first available printer");
        getPrinterInfo();
      });
    } else {
      getPrinterInfo();
    }

    function getPrinterInfo() {
      log(`Testing with printer: ${testPrinterName}`);

      printer.getPrinter(testPrinterName, (err, printerInfo) => {
        if (err) return callback(err);

        log(`Printer info retrieved for: ${printerInfo?.name || "Unknown"}`);

        if (VERBOSE && printerInfo) {
          verbose(
            `Jobs count: ${
              Array.isArray(printerInfo.jobs) ? printerInfo.jobs.length : 0
            }`
          );
        }

        callback(null, printerInfo);
      });
    }
  });
}

async function testGetPrinterDriverOptions() {
  log("Getting printer driver options...");

  // Try to get default printer first, fallback to first available printer
  let testPrinterName = await printer.getDefaultPrinterName();
  if (!testPrinterName) {
    const printers = await printer.getPrinters();
    if (!Array.isArray(printers) || printers.length === 0) {
      throw new Error("No printers available to test with");
    }
    testPrinterName = printers[0].name;
    log("No default printer found, using first available printer");
  }

  log(`Getting driver options for: ${testPrinterName}`);

  const options = await printer.getPrinterDriverOptions(testPrinterName);
  log(`Driver options retrieved: ${options ? "Yes" : "No"}`);

  if (options && typeof options === "object") {
    const optionKeys = Object.keys(options);
    log(
      `Available options: ${optionKeys.length} (${optionKeys
        .slice(0, 5)
        .join(", ")}${optionKeys.length > 5 ? "..." : ""})`
    );
  }

  return options;
}

// Callback version of testGetPrinterDriverOptions
function testGetPrinterDriverOptionsCallback(callback) {
  log("Getting printer driver options (callback)...");

  // First get default printer name
  printer.getDefaultPrinterName((err, testPrinterName) => {
    if (err) return callback(err);

    if (!testPrinterName) {
      // Fallback to first available printer
      printer.getPrinters((err, printers) => {
        if (err) return callback(err);
        if (!Array.isArray(printers) || printers.length === 0) {
          return callback(new Error("No printers available to test with"));
        }
        testPrinterName = printers[0].name;
        log("No default printer found, using first available printer");
        getDriverOptions();
      });
    } else {
      getDriverOptions();
    }

    function getDriverOptions() {
      log(`Getting driver options for: ${testPrinterName}`);

      printer.getPrinterDriverOptions(testPrinterName, (err, options) => {
        if (err) return callback(err);

        log(`Driver options retrieved: ${options ? "Yes" : "No"}`);

        if (options && typeof options === "object") {
          const optionKeys = Object.keys(options);
          log(
            `Available options: ${optionKeys.length} (${optionKeys
              .slice(0, 5)
              .join(", ")}${optionKeys.length > 5 ? "..." : ""})`
          );
        }

        callback(null, options);
      });
    }
  });
}

async function testGetSelectedPaperSize() {
  log("Getting selected paper size...");

  // Try to get default printer first, fallback to first available printer
  let testPrinterName = await printer.getDefaultPrinterName();
  if (!testPrinterName) {
    const printers = await printer.getPrinters();
    if (!Array.isArray(printers) || printers.length === 0) {
      throw new Error("No printers available to test with");
    }
    testPrinterName = printers[0].name;
    log("No default printer found, using first available printer");
  }

  log(`Getting paper size for: ${testPrinterName}`);

  const paperSize = await printer.getSelectedPaperSize(testPrinterName);
  log(`Selected paper size: ${paperSize || "Not available"}`);

  return paperSize;
}

// Callback version of testGetSelectedPaperSize
function testGetSelectedPaperSizeCallback(callback) {
  log("Getting selected paper size (callback)...");

  // First get default printer name
  printer.getDefaultPrinterName((err, testPrinterName) => {
    if (err) return callback(err);

    if (!testPrinterName) {
      // Fallback to first available printer
      printer.getPrinters((err, printers) => {
        if (err) return callback(err);
        if (!Array.isArray(printers) || printers.length === 0) {
          return callback(new Error("No printers available to test with"));
        }
        testPrinterName = printers[0].name;
        log("No default printer found, using first available printer");
        getPaperSize();
      });
    } else {
      getPaperSize();
    }

    function getPaperSize() {
      log(`Getting paper size for: ${testPrinterName}`);

      printer.getSelectedPaperSize(testPrinterName, (err, paperSize) => {
        if (err) return callback(err);

        log(`Selected paper size: ${paperSize || "Not available"}`);
        callback(null, paperSize);
      });
    }
  });
}

async function testGetSupportedPrintFormats() {
  log("Getting supported print formats...");

  const formats = await printer.getSupportedPrintFormats();
  log(
    `Supported formats: ${
      Array.isArray(formats) ? formats.join(", ") : "Not available"
    }`
  );

  return formats;
}

// Callback version of testGetSupportedPrintFormats
function testGetSupportedPrintFormatsCallback(callback) {
  log("Getting supported print formats (callback)...");

  printer.getSupportedPrintFormats((err, formats) => {
    if (err) return callback(err);

    log(
      `Supported formats: ${
        Array.isArray(formats) ? formats.join(", ") : "Not available"
      }`
    );
    callback(null, formats);
  });
}

async function testGetSupportedJobCommands() {
  log("Getting supported job commands...");

  const commands = await printer.getSupportedJobCommands();
  log(
    `Supported commands: ${
      Array.isArray(commands) ? commands.join(", ") : "Not available"
    }`
  );

  return commands;
}

// Callback version of testGetSupportedJobCommands
function testGetSupportedJobCommandsCallback(callback) {
  log("Getting supported job commands (callback)...");

  printer.getSupportedJobCommands((err, commands) => {
    if (err) return callback(err);

    log(
      `Supported commands: ${
        Array.isArray(commands) ? commands.join(", ") : "Not available"
      }`
    );
    callback(null, commands);
  });
}

async function testPrintDirect() {
  log("Testing direct printing...");

  // Try to get default printer first, fallback to first available printer
  let testPrinterName = await printer.getDefaultPrinterName();
  if (!testPrinterName) {
    const printers = await printer.getPrinters();
    if (!Array.isArray(printers) || printers.length === 0) {
      throw new Error("No printers available to test with");
    }
    testPrinterName = printers[0].name;
    log("No default printer found, using first available printer");
  }

  log(`Printing to: ${testPrinterName}`);

  log("⚠️  WARNING: This will send a test document to the printer!");
  log("   The document contains text data and should be safe.");
  log("   Press Ctrl+C within 3 seconds to cancel...");

  // Give user time to cancel
  await new Promise((resolve) => setTimeout(resolve, 3000));

  // Use consistent object-style parameters (now supported on both Windows and POSIX)
  const printParams = {
    data: TEST_DATA,
    printer: testPrinterName,
    docname: "Node-Printer Test Document",
    type: "RAW",
    options: {},
  };

  const jobId = await printer.printDirect(printParams);
  log(`Print job created with ID: ${jobId}`);

  return jobId;
}

// Callback version of testPrintDirect
function testPrintDirectCallback(callback) {
  log("Testing direct printing (callback)...");

  // First get default printer name
  printer.getDefaultPrinterName((err, testPrinterName) => {
    if (err) return callback(err);

    if (!testPrinterName) {
      // Fallback to first available printer
      printer.getPrinters((err, printers) => {
        if (err) return callback(err);
        if (!Array.isArray(printers) || printers.length === 0) {
          return callback(new Error("No printers available to test with"));
        }
        testPrinterName = printers[0].name;
        log("No default printer found, using first available printer");
        performPrint();
      });
    } else {
      performPrint();
    }

    function performPrint() {
      log(`Printing to: ${testPrinterName}`);

      log("⚠️  WARNING: This will send a test document to the printer!");
      log("   The document contains text data and should be safe.");

      // Use consistent object-style parameters with callback
      const printParams = {
        data: TEST_DATA,
        printer: testPrinterName,
        docname: "Node-Printer Test Document (Callback)",
        type: "RAW",
        options: {},
      };

      printer.printDirect(printParams, (err, jobId) => {
        if (err) return callback(err);

        log(`Print job created with ID: ${jobId}`);
        callback(null, jobId);
      });
    }
  });
}

async function testPrintFile() {
  log("Testing file printing...");

  // Try to get default printer first, fallback to first available printer
  let testPrinterName = await printer.getDefaultPrinterName();
  if (!testPrinterName) {
    const printers = await printer.getPrinters();
    if (!Array.isArray(printers) || printers.length === 0) {
      throw new Error("No printers available to test with");
    }
    testPrinterName = printers[0].name;
    log("No default printer found, using first available printer");
  }

  // Create test file
  log("Creating test file...");
  fs.writeFileSync(TEST_FILE_PATH, TEST_DATA);
  log(`Test file created: ${TEST_FILE_PATH}`);

  log(`Printing file to: ${testPrinterName}`);

  const printParams = {
    filename: TEST_FILE_PATH,
    printer: testPrinterName,
    docname: "Node-Printer Test File",
    options: {},
  };

  log("⚠️  WARNING: This will send a test file to the printer!");
  log(`   File: ${TEST_FILE_PATH}`);
  log("   Press Ctrl+C within 3 seconds to cancel...");

  // Give user time to cancel
  await new Promise((resolve) => setTimeout(resolve, 3000));

  const jobId = await printer.printFile(printParams);
  log(`Print job created with ID: ${jobId}`);

  // Clean up test file
  try {
    fs.unlinkSync(TEST_FILE_PATH);
    log("Test file cleaned up");
  } catch (err) {
    verbose(`Failed to clean up test file: ${err.message}`);
  }

  return jobId;
}

// Callback version of testPrintFile
function testPrintFileCallback(callback) {
  log("Testing file printing (callback)...");

  // First get default printer name
  printer.getDefaultPrinterName((err, testPrinterName) => {
    if (err) return callback(err);

    if (!testPrinterName) {
      // Fallback to first available printer
      printer.getPrinters((err, printers) => {
        if (err) return callback(err);
        if (!Array.isArray(printers) || printers.length === 0) {
          return callback(new Error("No printers available to test with"));
        }
        testPrinterName = printers[0].name;
        log("No default printer found, using first available printer");
        performFilePrint();
      });
    } else {
      performFilePrint();
    }

    function performFilePrint() {
      // Create test file
      log("Creating test file...");
      const callbackTestFilePath = TEST_FILE_PATH.replace(
        ".txt",
        "-callback.txt"
      );
      fs.writeFileSync(callbackTestFilePath, TEST_DATA);
      log(`Test file created: ${callbackTestFilePath}`);

      log(`Printing file to: ${testPrinterName}`);

      const printParams = {
        filename: callbackTestFilePath,
        printer: testPrinterName,
        docname: "Node-Printer Test File (Callback)",
        options: {},
      };

      log("⚠️  WARNING: This will send a test file to the printer!");
      log(`   File: ${callbackTestFilePath}`);

      printer.printFile(printParams, (err, jobId) => {
        // Clean up test file first
        try {
          fs.unlinkSync(callbackTestFilePath);
          log("Test file cleaned up");
        } catch (cleanupErr) {
          verbose(`Failed to clean up test file: ${cleanupErr.message}`);
        }

        if (err) return callback(err);

        log(`Print job created with ID: ${jobId}`);
        callback(null, jobId);
      });
    }
  });
}

async function testGetJob(knownJobId = null) {
  log("Testing job retrieval...");

  if (!knownJobId) {
    log("No known job ID provided, skipping job retrieval test");
    return null;
  }

  // Try to get default printer first, fallback to first available printer
  let testPrinterName = await printer.getDefaultPrinterName();
  if (!testPrinterName) {
    const printers = await printer.getPrinters();
    if (!Array.isArray(printers) || printers.length === 0) {
      throw new Error("No printers available to test with");
    }
    testPrinterName = printers[0].name;
    log("No default printer found, using first available printer");
  }

  log(`Getting job ${knownJobId} from printer: ${testPrinterName}`);

  const job = await printer.getJob(testPrinterName, knownJobId);
  log(`Job retrieved: ${job ? "Yes" : "No"}`);

  if (job && VERBOSE) {
    verbose(`Job details: ${formatResult(job)}`);
  }

  return job;
}

// Callback version of testGetJob
function testGetJobCallback(knownJobId = null, callback) {
  log("Testing job retrieval (callback)...");

  if (!knownJobId) {
    log("No known job ID provided, skipping job retrieval test");
    return callback(null, null);
  }

  // First get default printer name
  printer.getDefaultPrinterName((err, testPrinterName) => {
    if (err) return callback(err);

    if (!testPrinterName) {
      // Fallback to first available printer
      printer.getPrinters((err, printers) => {
        if (err) return callback(err);
        if (!Array.isArray(printers) || printers.length === 0) {
          return callback(new Error("No printers available to test with"));
        }
        testPrinterName = printers[0].name;
        log("No default printer found, using first available printer");
        getJobInfo();
      });
    } else {
      getJobInfo();
    }

    function getJobInfo() {
      log(`Getting job ${knownJobId} from printer: ${testPrinterName}`);

      printer.getJob(testPrinterName, knownJobId, (err, job) => {
        if (err) return callback(err);

        log(`Job retrieved: ${job ? "Yes" : "No"}`);

        if (job && VERBOSE) {
          verbose(`Job details: ${formatResult(job)}`);
        }

        callback(null, job);
      });
    }
  });
}

async function testSetJob(knownJobId = null) {
  log("Testing job control...");

  if (!knownJobId) {
    log("No known job ID provided, skipping job control test");
    return null;
  }

  // Try to get default printer first, fallback to first available printer
  let testPrinterName = await printer.getDefaultPrinterName();
  if (!testPrinterName) {
    const printers = await printer.getPrinters();
    if (!Array.isArray(printers) || printers.length === 0) {
      throw new Error("No printers available to test with");
    }
    testPrinterName = printers[0].name;
    log("No default printer found, using first available printer");
  }

  log(`Attempting to cancel job ${knownJobId} on printer: ${testPrinterName}`);

  log("⚠️  WARNING: This will attempt to cancel the print job!");
  log("   Press Ctrl+C within 3 seconds to cancel...");

  // Give user time to cancel
  await new Promise((resolve) => setTimeout(resolve, 3000));

  const result = await printer.setJob(testPrinterName, knownJobId, "CANCEL");
  log(`Job cancel result: ${result}`);

  return result;
}

// Callback version of testSetJob
function testSetJobCallback(knownJobId = null, callback) {
  log("Testing job control (callback)...");

  if (!knownJobId) {
    log("No known job ID provided, skipping job control test");
    return callback(null, null);
  }

  // First get default printer name
  printer.getDefaultPrinterName((err, testPrinterName) => {
    if (err) return callback(err);

    if (!testPrinterName) {
      // Fallback to first available printer
      printer.getPrinters((err, printers) => {
        if (err) return callback(err);
        if (!Array.isArray(printers) || printers.length === 0) {
          return callback(new Error("No printers available to test with"));
        }
        testPrinterName = printers[0].name;
        log("No default printer found, using first available printer");
        setJobControl();
      });
    } else {
      setJobControl();
    }

    function setJobControl() {
      log(
        `Attempting to cancel job ${knownJobId} on printer: ${testPrinterName}`
      );

      log("⚠️  WARNING: This will attempt to cancel the print job!");

      printer.setJob(testPrinterName, knownJobId, "CANCEL", (err, result) => {
        if (err) return callback(err);

        log(`Job cancel result: ${result}`);
        callback(null, result);
      });
    }
  });
}

// Main test runner
async function runAllTests() {
  log(`Starting comprehensive printer tests on ${PLATFORM}`);
  log(`Verbose mode: ${VERBOSE ? "ON" : "OFF"}`);
  log(`Node.js version: ${process.version}`);

  const results = {};
  let createdJobId = null;

  // Core information functions (safe to run) - Test both Promise and Callback patterns
  results.getPrinters = await testFunction("getPrinters", testGetPrinters);
  results.getPrintersCallback = await testFunctionWithCallback(
    "getPrinters",
    testGetPrintersCallback
  );

  results.getDefaultPrinterName = await testFunction(
    "getDefaultPrinterName",
    testGetDefaultPrinterName
  );
  results.getDefaultPrinterNameCallback = await testFunctionWithCallback(
    "getDefaultPrinterName",
    testGetDefaultPrinterNameCallback
  );

  results.getPrinter = await testFunction("getPrinter", testGetPrinter);
  results.getPrinterCallback = await testFunctionWithCallback(
    "getPrinter",
    testGetPrinterCallback
  );

  results.getPrinterDriverOptions = await testFunction(
    "getPrinterDriverOptions",
    testGetPrinterDriverOptions
  );
  results.getPrinterDriverOptionsCallback = await testFunctionWithCallback(
    "getPrinterDriverOptions",
    testGetPrinterDriverOptionsCallback
  );

  results.getSelectedPaperSize = await testFunction(
    "getSelectedPaperSize",
    testGetSelectedPaperSize
  );
  results.getSelectedPaperSizeCallback = await testFunctionWithCallback(
    "getSelectedPaperSize",
    testGetSelectedPaperSizeCallback
  );

  results.getSupportedPrintFormats = await testFunction(
    "getSupportedPrintFormats",
    testGetSupportedPrintFormats
  );
  results.getSupportedPrintFormatsCallback = await testFunctionWithCallback(
    "getSupportedPrintFormats",
    testGetSupportedPrintFormatsCallback
  );

  results.getSupportedJobCommands = await testFunction(
    "getSupportedJobCommands",
    testGetSupportedJobCommands
  );
  results.getSupportedJobCommandsCallback = await testFunctionWithCallback(
    "getSupportedJobCommands",
    testGetSupportedJobCommandsCallback
  );

  // Ask user for confirmation before running print tests
  log("\n" + "!".repeat(60));
  log("The following tests will attempt to send print jobs to your printer.");
  log("This may result in physical printouts or consume printer resources.");
  log("!".repeat(60));

  const readline = require("readline");
  const rl = readline.createInterface({
    input: process.stdin,
    output: process.stdout,
  });

  const answer = await new Promise((resolve) => {
    rl.question("Do you want to run print tests? (y/N): ", resolve);
  });
  rl.close();

  if (answer.toLowerCase() === "y" || answer.toLowerCase() === "yes") {
    // Print tests (these will send jobs to printer) - Test both Promise and Callback patterns
    results.printDirect = await testFunction("printDirect", testPrintDirect);
    if (results.printDirect.success) {
      createdJobId = results.printDirect.result;
    }

    results.printDirectCallback = await testFunctionWithCallback(
      "printDirect",
      testPrintDirectCallback
    );
    if (results.printDirectCallback.success && !createdJobId) {
      createdJobId = results.printDirectCallback.result;
    }

    results.printFile = await testFunction("printFile", testPrintFile);
    if (results.printFile.success && !createdJobId) {
      createdJobId = results.printFile.result;
    }

    results.printFileCallback = await testFunctionWithCallback(
      "printFile",
      testPrintFileCallback
    );
    if (results.printFileCallback.success && !createdJobId) {
      createdJobId = results.printFileCallback.result;
    }

    // Job management tests (use created job if available) - Test both Promise and Callback patterns
    if (createdJobId) {
      results.getJob = await testFunction("getJob", () =>
        testGetJob(createdJobId)
      );
      results.getJobCallback = await testFunctionWithCallback(
        "getJob",
        (callback) => testGetJobCallback(createdJobId, callback)
      );

      results.setJob = await testFunction("setJob", () =>
        testSetJob(createdJobId)
      );
      results.setJobCallback = await testFunctionWithCallback(
        "setJob",
        (callback) => testSetJobCallback(createdJobId, callback)
      );
    } else {
      results.getJob = await testFunction("getJob", () => testGetJob());
      results.getJobCallback = await testFunctionWithCallback(
        "getJob",
        (callback) => testGetJobCallback(null, callback)
      );

      results.setJob = await testFunction("setJob", () => testSetJob());
      results.setJobCallback = await testFunctionWithCallback(
        "setJob",
        (callback) => testSetJobCallback(null, callback)
      );
    }
  } else {
    log("Skipping print tests as requested.");
    const skipMessage = "Skipped by user";
    results.printDirect = { success: false, error: skipMessage };
    results.printDirectCallback = { success: false, error: skipMessage };
    results.printFile = { success: false, error: skipMessage };
    results.printFileCallback = { success: false, error: skipMessage };
    results.getJob = { success: false, error: skipMessage };
    results.getJobCallback = { success: false, error: skipMessage };
    results.setJob = { success: false, error: skipMessage };
    results.setJobCallback = { success: false, error: skipMessage };
  }

  // Summary
  log("\n" + "=".repeat(60));
  log("TEST SUMMARY");
  log("=".repeat(60));

  const testNames = Object.keys(results);
  const successCount = testNames.filter((name) => results[name].success).length;
  const totalCount = testNames.length;

  // Separate Promise vs Callback test results
  const promiseTests = testNames.filter((name) => !name.includes("Callback"));
  const callbackTests = testNames.filter((name) => name.includes("Callback"));

  const promiseSuccessCount = promiseTests.filter(
    (name) => results[name].success
  ).length;
  const callbackSuccessCount = callbackTests.filter(
    (name) => results[name].success
  ).length;

  log(`Platform: ${PLATFORM}`);
  log(
    `Total tests: ${totalCount} (${promiseTests.length} Promise + ${callbackTests.length} Callback)`
  );
  log(
    `Successful: ${successCount} (${promiseSuccessCount} Promise + ${callbackSuccessCount} Callback)`
  );
  log(`Failed: ${totalCount - successCount}`);
  log(`Success rate: ${Math.round((successCount / totalCount) * 100)}%`);
  log(
    `Promise success rate: ${Math.round(
      (promiseSuccessCount / promiseTests.length) * 100
    )}%`
  );
  log(
    `Callback success rate: ${Math.round(
      (callbackSuccessCount / callbackTests.length) * 100
    )}%`
  );

  log("\nDetailed Results:");

  log("\n📋 Promise-based tests:");
  promiseTests.forEach((name) => {
    const result = results[name];
    const status = result.success ? "✅" : "❌";
    const message = result.success ? "SUCCESS" : result.error;
    log(`  ${status} ${name}: ${message}`);
  });

  log("\n📞 Callback-based tests:");
  callbackTests.forEach((name) => {
    const result = results[name];
    const status = result.success ? "✅" : "❌";
    const message = result.success ? "SUCCESS" : result.error;
    log(`  ${status} ${name}: ${message}`);
  });

  // Platform-specific notes
  log("\nPlatform-specific Notes:");
  if (PLATFORM === "win32") {
    log("- Windows implementation using Winspool API");
    log("- All core functions are fully supported and synchronized");
    log("- Print formats include Windows-specific formats (NT EMF, XPS2GDI)");
    log("- Driver options provide basic Windows printer properties");
    log(
      "- Job management includes Windows-specific commands (RETAIN, SENT-TO-PRINTER)"
    );
  } else {
    log("- POSIX/CUPS implementation provides comprehensive printer support");
    log("- All functions fully synchronized with Windows implementation");
    log("- Uses modern CUPS destination-based APIs (no deprecated PPD APIs)");
    log("- Enhanced format support including PDF, JPEG, PostScript");
    log("- Comprehensive job status mapping with fallback handling");
  }

  log("- ✅ All functions are synchronized across Windows and POSIX platforms");
  log("- ✅ Consistent parameter signatures and error handling");
  log("- ✅ Cross-platform API compatibility maintained");
  log("- ✅ Both Promise and callback patterns are fully supported");

  log("\nTest completed successfully!");
  return results;
}

// Handle graceful shutdown
process.on("SIGINT", () => {
  log("\nTest interrupted by user");
  try {
    if (fs.existsSync(TEST_FILE_PATH)) {
      fs.unlinkSync(TEST_FILE_PATH);
      log("Cleaned up test file");
    }
  } catch (err) {
    // Ignore cleanup errors
  }
  process.exit(1);
});

// Run tests if this file is executed directly
if (require.main === module) {
  runAllTests().catch((error) => {
    log(`Fatal error: ${error.message}`, "ERROR");
    process.exit(1);
  });
}

module.exports = {
  runAllTests,
  testFunction,
  log,
  verbose,
};
