const t = require('tap');
const { __test__ } = require('../printer');
const { toDateOrNull, normalizeJob } = __test__;

t.test('time handling', (t) => {
  // 1) null/0 values -> null
  t.equal(toDateOrNull(0), null, '0 should become null');
  t.equal(toDateOrNull(null), null, 'null should remain null');
  t.equal(toDateOrNull(undefined), null, 'undefined should become null');

  // 2) numeric seconds -> Date in ms
  const sec = 1694764800; // 2023-09-15T00:00:00Z approx
  const d = toDateOrNull(sec);
  t.type(d, Date, 'toDateOrNull should return a Date for numeric input');
  t.equal(Math.floor(d.getTime() / 1000), sec, 'Date seconds must match original seconds');

  // 3) normalizeJob converts numeric fields
  const job = { creationTime: sec, processingTime: 0, completedTime: null };
  const norm = normalizeJob(job);
  t.type(norm.creationTime, Date, 'creationTime should be Date');
  t.equal(norm.processingTime, null, 'processingTime 0 -> null');
  t.equal(norm.completedTime, null, 'completedTime null -> null');

  t.end();
});
