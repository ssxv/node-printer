const t = require('tap');
const binding = require('./_mocks/printer-mock');

t.test('error handling', async (t) => {
  // 1) printDirect with invalid printer should reject with Error
  try {
    await binding.printDirect(Buffer.from('hello'), 'Printer-Does-Not-Exist-XYZ-123', 'doc', 'RAW');
    t.fail('printDirect should have rejected for invalid printer');
  } catch (err) {
    t.type(err, Error, 'printDirect rejection is an Error');
    t.match(err.message, /mock printer invalid/, 'error message mentions mock printer invalid');
  }

  // 2) printFile with non-existent file should throw synchronously
  try {
    binding.printFile('non-existent-file.xyz', 'doc', 'Printer-Does-Not-Exist-XYZ-123');
    t.fail('binding.printFile should have thrown for non-existent file');
  } catch (err) {
    t.type(err, Error, 'binding.printFile threw an Error for non-existent file');
    t.match(err.message, /cannot open file/, 'error message contains cannot open file');
  }

  // 3) printFile with valid file but invalid printer should reject
  const tmp = require('os').tmpdir() + '\\node_printer_test_tmp.txt';
  const fs = require('fs');
  fs.writeFileSync(tmp, 'hello world');
  try {
    await binding.printFile(tmp, 'doc', 'Printer-Does-Not-Exist-XYZ-123');
    t.fail('printFile should have rejected for invalid printer');
  } catch (err) {
    t.type(err, Error, 'printFile rejection is an Error');
    t.match(err.message, /mock printer invalid/, 'error message mentions mock printer invalid');
  } finally {
    try { fs.unlinkSync(tmp); } catch (e) {}
  }

  t.end();
});
