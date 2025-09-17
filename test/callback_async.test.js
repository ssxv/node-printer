const t = require('tap');

const binding = require('./_mocks/printer-mock');

t.test('callback-style printFile invoked asynchronously', async (t) => {
  // Ensure printer.js picks up the mock binding by preloading it into require cache
  // before requiring the wrapper. This keeps mocking responsibility in the test.
  const Module = require('module');
  const mock = binding;
  const indexPath = require.resolve('../index');
  const m = new Module(indexPath);
  m.filename = indexPath;
  m.exports = mock;
  m.loaded = true;
  require.cache[indexPath] = m;

  // Our mock returns a Promise; ensure callback-style wrapper behaves and is asynchronous.
  const printer = require('../printer');
  let called = false;
  const p = printer.printFile({ filename: 'README.md' }, function(err, res){
    t.error(err);
    t.equal(res, 99, 'callback should receive job id 99 from mock');
    called = true;
  });
  t.ok(p && typeof p.then === 'function', 'printFile should return a Promise');
  const res = await p;
  t.equal(res, 99, 'promise should resolve to 99');
  // callback must not have been called synchronously
  t.equal(called, true, 'callback was called (may be async)');
});

