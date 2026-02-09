const t = require('tap');

const binding = require('../_mocks/printer-mock');

t.test('no double reporting between promise and callback', async (t) => {
  // Preload the mock into require cache so the wrapper picks it up.
  const Module = require('module');
  const mock = binding;
  const indexPath = require.resolve('../../binding');
  const m = new Module(indexPath);
  m.filename = indexPath;
  m.exports = mock;
  m.loaded = true;
  require.cache[indexPath] = m;

  const printer = require('../../printer');
  let callbackCalled = 0;
  const p = printer.printFile({ filename: 'README.md' }, function(err, res){
    t.error(err);
    t.equal(res, 99);
    callbackCalled++;
  });
  const res = await p;
  t.equal(res, 99);
  // allow next tick for callback
  await new Promise((r) => setImmediate(r));
  t.equal(callbackCalled, 1, 'callback should be called exactly once');
});
