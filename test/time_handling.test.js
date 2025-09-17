// Tests for time handling: ensure numeric seconds -> Date or null conversion is deterministic
const { __test__ } = require('../printer');
const { toDateOrNull, normalizeJob } = __test__;

function assert(condition, msg) {
  if (!condition) throw new Error(msg || 'Assertion failed');
}

(function run() {
  // 1) null/0 values -> null
  assert(toDateOrNull(0) === null, '0 should become null');
  assert(toDateOrNull(null) === null, 'null should remain null');
  assert(toDateOrNull(undefined) === null, 'undefined should become null');

  // 2) numeric seconds -> Date in ms
  const sec = 1694764800; // 2023-09-15T00:00:00Z approx
  const d = toDateOrNull(sec);
  assert(d instanceof Date, 'toDateOrNull should return a Date for numeric input');
  assert(Math.floor(d.getTime() / 1000) === sec, 'Date seconds must match original seconds');

  // 3) normalizeJob converts numeric fields
  const job = { creationTime: sec, processingTime: 0, completedTime: null };
  const norm = normalizeJob(job);
  assert(norm.creationTime instanceof Date, 'creationTime should be Date');
  assert(norm.processingTime === null, 'processingTime 0 -> null');
  assert(norm.completedTime === null, 'completedTime null -> null');

  console.log('All time handling tests passed');
})();
