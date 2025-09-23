# Testing All Functions - Usage Guide

This directory contains `testAllFunctions.js`, a comprehensive test script for all functions available in the `@ssxv/node-printer` package.

## Usage

```bash
# Basic test run
node testAllFunctions.js

# Verbose output (shows detailed results)
node testAllFunctions.js --verbose
```

## What the Script Tests

The script tests all exported functions from `printer.js`:

### Information Functions (Safe - No Physical Output)
1. **getPrinters()** - Lists all available printers
2. **getDefaultPrinterName()** - Gets the default printer name
3. **getPrinter(name)** - Gets detailed info for a specific printer
4. **getPrinterDriverOptions(name)** - Gets driver options for a printer
5. **getSelectedPaperSize(name)** - Gets the selected paper size
6. **getSupportedPrintFormats()** - Lists supported print formats
7. **getSupportedJobCommands()** - Lists supported job commands

### Print Functions (⚠️ May Produce Physical Output)
8. **printDirect(params)** - Prints data directly to printer
9. **printFile(params)** - Prints a file to printer

### Job Management Functions
10. **getJob(printerName, jobId)** - Gets information about a print job
11. **setJob(printerName, jobId, command)** - Controls print jobs (e.g., cancel)

## Safety Features

- **Interactive Confirmation**: The script asks for confirmation before running print tests
- **3-Second Delays**: Gives you time to cancel potentially destructive operations
- **Graceful Cleanup**: Removes temporary files even if interrupted
- **Error Handling**: Continues testing even if individual functions fail

## Platform Compatibility

Both Windows and POSIX platforms are fully supported with synchronized implementations:

### Windows
- Uses Winspool API for comprehensive printer management
- **Full function support** - all 11 functions implemented and tested
- Enhanced job management with Windows-specific commands
- Supports both local and network printers

### Linux/POSIX  
- Uses CUPS (Common Unix Printing System) for robust printer support
- **Full function support** - all 11 functions implemented and tested
- Modern CUPS APIs (avoiding deprecated PPD functions)
- Requires libcups >= 1.6 for optimal functionality

### Cross-Platform Features
- ✅ **Synchronized APIs**: Identical function signatures across platforms
- ✅ **Consistent Error Handling**: Standardized error messages and validation
- ✅ **Parameter Compatibility**: Same parameter formats supported on both platforms
- ✅ **Feature Parity**: All core functionality available on both Windows and POSIX

## Expected Output

### Successful Run Example
```
[2025-09-24T...] [INFO] Starting comprehensive printer tests on win32
[2025-09-24T...] [INFO] Verbose mode: OFF
[2025-09-24T...] [INFO] Node.js version: v22.17.1

==================================================
Testing: getPrinters
==================================================
[2025-09-24T...] [INFO] Getting list of all printers...
[2025-09-24T...] [INFO] Found 3 printers
[2025-09-24T...] [INFO] Printer 1: Microsoft Print to PDF (Default: false)
[2025-09-24T...] [INFO] Printer 2: HP LaserJet Pro (Default: false)
[2025-09-24T...] [INFO] Printer 3: Everycom-80-Series (Default: true)
[2025-09-24T...] [INFO] ✅ getPrinters - SUCCESS

... (continues for all functions)

============================================================
Do you want to run print tests? (y/N): y
============================================================

==================================================
Testing: printDirect
==================================================
[2025-09-24T...] [INFO] Print job created with ID: 10
[2025-09-24T...] [INFO] ✅ printDirect - SUCCESS

============================================================
TEST SUMMARY
============================================================
[2025-09-24T...] [INFO] Platform: win32
[2025-09-24T...] [INFO] Total tests: 11
[2025-09-24T...] [INFO] Successful: 11
[2025-09-24T...] [INFO] Failed: 0
[2025-09-24T...] [INFO] Success rate: 100%

Detailed Results:
  ✅ getPrinters: SUCCESS
  ✅ getDefaultPrinterName: SUCCESS
  ✅ getPrinter: SUCCESS
  ✅ getPrinterDriverOptions: SUCCESS
  ✅ getSelectedPaperSize: SUCCESS
  ✅ getSupportedPrintFormats: SUCCESS
  ✅ getSupportedJobCommands: SUCCESS
  ✅ printDirect: SUCCESS
  ✅ printFile: SUCCESS
  ✅ getJob: SUCCESS
  ✅ setJob: SUCCESS

Platform-specific Notes:
- Windows implementation using Winspool API
- All core functions are fully supported and synchronized
- ✅ All functions are synchronized across Windows and POSIX platforms
- ✅ Consistent parameter signatures and error handling
- ✅ Cross-platform API compatibility maintained

Test completed successfully!
```

## Troubleshooting

### No Printers Found
- **Windows**: Check that printers are installed and accessible via Control Panel
- **Linux**: Ensure CUPS is running (`systemctl status cups`)
- Both: Verify printer drivers are properly installed

### Build Errors
- **Windows**: Install Visual Studio Build Tools or Visual Studio with C++ support
- **Linux**: Install development packages (`sudo apt-get install libcups2-dev build-essential`)

### Permission Issues
- **Linux**: User may need to be in `lpadmin` group (`sudo usermod -a -G lpadmin $USER`)
- **Windows**: Run as administrator if accessing network printers or system printers
- Both: Check firewall settings for network printers

### Function Not Working
- **All Platforms**: All functions are now synchronized and should work identically
- Check printer status and connectivity
- Verify printer is not offline or in error state
- For print tests, ensure printer has paper and is ready

## Test File Creation

The script creates a temporary test file (`test-document.txt`) containing:
```
Hello World!
This is a test document from node-printer.
Timestamp: 2025-09-24T...
```

This file is automatically cleaned up after testing.

## Synchronization Status

The `@ssxv/node-printer` package now features **full synchronization** between Windows and POSIX implementations:

### ✅ Complete Function Parity
- All 11 core functions available on both platforms
- No platform-specific limitations or "not supported" errors
- Identical API signatures across Windows and POSIX

### ✅ Consistent Parameter Handling  
- `printDirect`: Supports object-style parameters on both platforms
- `printFile`: Consistent 3+ parameter requirement (filename, docname, printer, options)
- All functions: Standardized parameter validation and error messages

### ✅ Unified Error Handling
- Consistent error messages across platforms
- Standardized parameter validation
- Same error types and codes

### ✅ Feature Synchronization
- **Windows**: Enhanced from basic implementation to full feature set
- **POSIX**: Modernized to use non-deprecated CUPS APIs  
- **Both**: Support for job management, driver options, and format detection

### Expected Test Results
With full synchronization, you should expect **100% success rate** on both platforms when all functions are tested.

## Customization

You can modify the script to:
- Change test data content
- Add specific printer names to test
- Skip certain tests
- Add custom validation logic

## Safety Notes

⚠️ **Print Tests Warning**: The print functions will send actual print jobs to your printer. This may:
- Consume paper and ink/toner
- Generate physical printouts
- Use printer queue resources

The script will always ask for confirmation and provide cancellation opportunities before running these tests.