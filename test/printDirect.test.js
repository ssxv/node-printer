const t = require('tap');

// Use the mock binding for unit tests to avoid relying on native addon presence
const binding = require('./_mocks/printer-mock');

async function assertThrowsOrRejects(t, fn) {
  try {
    const res = fn();
    if (res && typeof res.then === 'function') {
      await t.rejects(res, { instanceOf: Error });
    } else {
      t.fail('Expected function to throw or return rejecting promise but it returned: ' + String(res));
    }
  } catch (e) {
    t.ok(e instanceof Error, 'synchronous throw should be an Error');
  }
}

t.test('printDirect rejects/throws for invalid printer', async (t) => {
  const opts = { data: Buffer.from('hello'), printer: 'invalid', docname: 'd', type: 'RAW' };
  try {
    await binding.printDirect(opts);
    t.fail('Expected printDirect to reject for invalid printer');
  } catch (e) {
    t.ok(e instanceof Error, 'should reject with Error');
  }
});

t.test('printDirect resolves for valid printer', async (t) => {
  const opts = { data: Buffer.from('hello'), printer: 'valid', docname: 'd', type: 'RAW' };
  const res = await binding.printDirect(opts);
  t.equal(res, 42);
});
