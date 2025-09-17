const t = require('tap');

// This test exercises the large-buffer path. The C++ addon uses a STREAM_THRESHOLD
// (4 MiB) and for large buffers may write to a temp file and stream inside the
// worker. For unit tests we use the mock by default; on Windows you can run the
// native integration by setting RUN_INTEGRATION=1 in CI or locally.

const isWindows = process.platform === 'win32';
const useIntegration = isWindows && process.env.RUN_INTEGRATION === '1';
const binding = useIntegration ? require('..') : require('./_mocks/printer-mock');

if (!useIntegration) {
  // Preload mock in require cache so the wrapper picks it up when requiring ../printer
  const Module = require('module');
  const mock = binding;
  const indexPath = require.resolve('../index');
  const m = new Module(indexPath);
  m.filename = indexPath;
  m.exports = mock;
  m.loaded = true;
  require.cache[indexPath] = m;
}

const printer = require('../printer');

t.test('printDirect handles large buffers (mock or native integration)', async (t) => {
  // Construct a buffer slightly larger than 4 MiB (4 * 1024 * 1024)
  const largeSize = 4 * 1024 * 1024 + 16;
  const buf = Buffer.alloc(largeSize, 0x41);

  // Use wrapper API to exercise both the JS wrapper and binding layer.
  // For mocks, we expect 42; for the real addon, we accept any numeric job id.
  const opts = { data: buf, printer: 'valid', docname: 'bigjob', type: 'RAW' };
  const res = await printer.printDirect(opts);

  if (useIntegration) {
    t.type(res, 'number', 'integration: native addon should return a numeric job id');
  } else {
    t.equal(res, 42, 'mock should return job id 42');
  }
});
