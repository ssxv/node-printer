const addon = require('../build/Release/node_printer');
function run() {
  const printers = addon.getPrinters();
  if (!Array.isArray(printers)) {
    console.error('FAIL: getPrinters did not return an array');
    process.exit(2);
  }
  if (printers.length === 0) {
    console.warn('WARN: getPrinters returned an empty array');
  } else {
    const p = printers[0];
    if (!p.name) {
      console.error('FAIL: first printer missing name field', p);
      process.exit(3);
    }
    if (typeof p.isDefault !== 'boolean') {
      console.error('FAIL: first printer missing isDefault boolean field', p);
      process.exit(4);
    }
    if (!p.options || typeof p.options !== 'object') {
      console.error('FAIL: first printer missing options object', p);
      process.exit(5);
    }
    if (!('printer-state' in p.options)) {
      console.error("FAIL: options missing 'printer-state'", p);
      process.exit(6);
    }
    console.log('PASS: getPrinters returned', printers.length, 'printers. First printer:', p.name);
  }
  // Print a summary of keys in the first printer for comparison
  if (printers.length > 0) {
    console.log('Printer keys:', Object.keys(printers[0]));
  }
}
run();
