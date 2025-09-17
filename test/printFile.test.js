const t = require('tap');

const binding = require('./_mocks/printer-mock');

async function assertThrowsOrRejects(t, fn) {
  try {
    const res = fn();
    if (res && typeof res.then === 'function') {
      await t.rejects(res, { instanceOf: Error });
    } else {
      // If the function returned synchronously (non-promise) and didn't throw,
      // that's an unexpected success.
      t.fail('Expected function to throw or return rejecting promise but it returned: ' + String(res));
    }
  } catch (e) {
    t.ok(e instanceof Error, 'synchronous throw should be an Error');
  }
}

t.test('printFile rejects/throws for non-existent file', async (t) => {
  await assertThrowsOrRejects(t, () => binding.printFile('non-existent-file.xyz'));
});

t.test('printFile resolves for valid file', async (t) => {
  const res = await binding.printFile('somefile.txt', 'doc', 'valid');
  t.equal(res, 99);
});
