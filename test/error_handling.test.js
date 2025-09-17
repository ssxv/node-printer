// Minimal tests for native error handling (no test framework)
// Use the test mock binding to exercise error handling deterministically
const binding = require('./_mocks/printer-mock');

function expectRejectPromise(promise) {
  return promise.then(
    () => {
      console.error('Expected promise to reject but it resolved');
      process.exitCode = 2;
    },
    (err) => {
      if (!(err instanceof Error)) {
        console.error('Rejected value is not an Error:', err);
        process.exitCode = 3;
        return;
      }
      if (typeof err.code !== 'number') {
        console.error('Error.code is not a number:', err);
        process.exitCode = 4;
        return;
      }
      if (!err.message || typeof err.message !== 'string') {
        console.error('Error.message is empty or not a string:', err);
        process.exitCode = 5;
        return;
      }
      console.log('OK: got Error with numeric code and message -', err.code, err.message);
    }
  );
}

(async function run() {
  // 1) printDirect with invalid printer
  try {
    const invalidPrinter = 'Printer-Does-Not-Exist-XYZ-123';
    // call native binding.printDirect with positional args
    await expectRejectPromise(binding.printDirect(Buffer.from('hello'), invalidPrinter, 'doc', 'RAW'));
  } catch (e) {
    console.error('Unexpected exception during printDirect test:', e);
    process.exitCode = 10;
  }

  // 2) printFile with invalid file path -> should error opening file synchronously in JS
  try {
    // calling native binding.printFile on a non-existent file results in a synchronous throw
    try {
      binding.printFile('non-existent-file.xyz', 'doc', 'Printer-Does-Not-Exist-XYZ-123');
      console.error('Expected binding.printFile to throw synchronously for non-existent file');
      process.exitCode = 6;
    } catch (err) {
      if (!(err instanceof Error)) {
        console.error('Synchronous throw from binding.printFile is not an Error:', err);
        process.exitCode = 7;
      } else {
        console.log('OK: binding.printFile threw synchronously for non-existent file:', err.message);
      }
    }
  } catch (e) {
    console.error('Unexpected exception during printFile test:', e);
    process.exitCode = 11;
  }

  // 3) printFile with invalid printer but valid file: create a small temp file
  const tmp = require('os').tmpdir() + '\\node_printer_test_tmp.txt';
  const fs = require('fs');
  try {
  fs.writeFileSync(tmp, 'hello world');
  await expectRejectPromise(binding.printFile(tmp, 'doc', 'Printer-Does-Not-Exist-XYZ-123'));
  } catch (e) {
    console.error('Unexpected exception during printFile(2) test:', e);
    process.exitCode = 12;
  } finally {
    try { fs.unlinkSync(tmp); } catch (e) {}
  }

  if (!process.exitCode) console.log('All error-handling tests completed (see OK lines)');
})();
