const t = require('tap');

// Integration test: only run on Windows and when RUN_INTEGRATION=1 is set.
if (process.platform !== 'win32' || process.env.RUN_INTEGRATION !== '1') {
  t.test('skip get_printers integration', (t) => {
    t.skip('integration test skipped; set RUN_INTEGRATION=1 on Windows to run');
    t.end();
  });
} else {
  t.test('getPrinters integration', (t) => {
    const addon = require('../build/Release/node_printer');
    const printers = addon.getPrinters();
    t.ok(Array.isArray(printers), 'getPrinters should return an array');
    if (printers.length > 0) {
      const p = printers[0];
      t.ok(p.name, 'printer should have name');
      t.type(p.isDefault, 'boolean', 'isDefault should be boolean');
      t.type(p.options, 'object', 'options should be an object');
      t.ok('printer-state' in p.options, "options should contain 'printer-state'");
    } else {
      t.comment('No printers found on system (WARN)');
    }
    t.end();
  });
}
