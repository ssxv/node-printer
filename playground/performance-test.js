#!/usr/bin/env node
/**
 * Performance test - Test performance characteristics
 * Run with: node performance-test.js
 */

console.log('=== Performance Test ===\n');

function measureTime(label, fn) {
  return async (...args) => {
    const start = process.hrtime.bigint();
    try {
      const result = await fn(...args);
      const end = process.hrtime.bigint();
      const duration = Number(end - start) / 1000000; // Convert to milliseconds
      console.log(`⏱️  ${label}: ${duration.toFixed(2)}ms`);
      return result;
    } catch (error) {
      const end = process.hrtime.bigint();
      const duration = Number(end - start) / 1000000;
      console.log(`⏱️  ${label}: ${duration.toFixed(2)}ms (failed: ${error.message})`);
      throw error;
    }
  };
}

async function testPerformance() {
  console.log('🚀 Running performance tests...\n');

  try {
    const { printers, jobs } = require('@ssxv/node-printer');
    const printer = require('@ssxv/node-printer/printer');

    // Test 1: Printer listing performance
    console.log('📊 Test 1: Printer listing performance');
    
    const measureList = measureTime('Modern API - printers.list()', printers.list);
    const modernResult = await measureList();
    
    const measureLegacyList = measureTime('Legacy API - getPrinters()', printer.getPrinters);
    const legacyResult = await measureLegacyList();
    
    console.log(`Results: Modern API found ${modernResult.length} printers, Legacy API found ${legacyResult.length} printers\n`);

    // Test 2: Multiple rapid printer list calls
    console.log('📊 Test 2: Rapid successive calls (10x)');
    const rapidStart = process.hrtime.bigint();
    
    const promises = [];
    for (let i = 0; i < 10; i++) {
      promises.push(printers.list());
    }
    
    const results = await Promise.all(promises);
    const rapidEnd = process.hrtime.bigint();
    const totalTime = Number(rapidEnd - rapidStart) / 1000000;
    const avgTime = totalTime / 10;
    
    console.log(`⏱️  10 parallel calls: ${totalTime.toFixed(2)}ms total, ${avgTime.toFixed(2)}ms average\n`);

    // Test 3: Default printer lookup performance
    console.log('📊 Test 3: Default printer lookup');
    try {
      const measureDefault = measureTime('Default printer lookup', printers.default);
      const defaultPrinter = await measureDefault();
      console.log(`✅ Default printer: ${defaultPrinter.name}\n`);
    } catch (error) {
      console.log(`⚠️  No default printer available: ${error.message}\n`);
    }

    // Test 4: Memory usage before and after operations
    console.log('📊 Test 4: Memory usage');
    const memBefore = process.memoryUsage();
    console.log(`Memory before tests:`, {
      rss: `${(memBefore.rss / 1024 / 1024).toFixed(2)} MB`,
      heapUsed: `${(memBefore.heapUsed / 1024 / 1024).toFixed(2)} MB`,
      heapTotal: `${(memBefore.heapTotal / 1024 / 1024).toFixed(2)} MB`
    });

    // Perform some operations
    for (let i = 0; i < 5; i++) {
      await printers.list();
    }

    const memAfter = process.memoryUsage();
    console.log(`Memory after tests:`, {
      rss: `${(memAfter.rss / 1024 / 1024).toFixed(2)} MB`,
      heapUsed: `${(memAfter.heapUsed / 1024 / 1024).toFixed(2)} MB`,
      heapTotal: `${(memAfter.heapTotal / 1024 / 1024).toFixed(2)} MB`
    });

    const rssIncrease = (memAfter.rss - memBefore.rss) / 1024 / 1024;
    const heapIncrease = (memAfter.heapUsed - memBefore.heapUsed) / 1024 / 1024;
    
    console.log(`Memory changes:`, {
      rss: `${rssIncrease > 0 ? '+' : ''}${rssIncrease.toFixed(2)} MB`,
      heap: `${heapIncrease > 0 ? '+' : ''}${heapIncrease.toFixed(2)} MB`
    });

    // Test 5: Large data handling (simulated)
    console.log('\n📊 Test 5: Large data preparation');
    const sizes = [1024, 10240, 102400]; // 1KB, 10KB, 100KB
    
    for (const size of sizes) {
      const data = Buffer.alloc(size, 'A');
      const start = process.hrtime.bigint();
      
      // Simulate what would happen in a print job (just the data preparation)
      const processedData = Buffer.from(data);
      const docname = `Performance test ${size} bytes`;
      
      const end = process.hrtime.bigint();
      const duration = Number(end - start) / 1000000;
      
      console.log(`⏱️  Prepare ${size} bytes: ${duration.toFixed(2)}ms`);
    }

    console.log('\n✅ Performance tests completed!');
    
    // Test 6: Cleanup and final memory check
    if (global.gc) {
      console.log('\n🧹 Running garbage collection...');
      global.gc();
      const memFinal = process.memoryUsage();
      console.log(`Final memory:`, {
        rss: `${(memFinal.rss / 1024 / 1024).toFixed(2)} MB`,
        heapUsed: `${(memFinal.heapUsed / 1024 / 1024).toFixed(2)} MB`
      });
    } else {
      console.log('\n💡 Run with --expose-gc to see garbage collection impact');
    }

  } catch (error) {
    console.error('❌ Performance test failed:', error.message);
    console.error(error.stack);
  }
}

if (require.main === module) {
  testPerformance().catch(console.error);
}