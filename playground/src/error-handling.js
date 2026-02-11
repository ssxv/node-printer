#!/usr/bin/env node
/**
 * Error handling test - Test error scenarios and edge cases
 * Run with: node error-handling.js
 */
const { printers, jobs, PrinterError } = require('@ssxv/node-printer');

console.log('=== Error Handling Test ===\n');

async function testErrorHandling() {
  console.log('🧪 Testing various error scenarios...\n');

  // Test 1: Invalid printer name
  console.log('❌ Test 1: Invalid printer name');
  try {
    await jobs.printRaw({
      data: Buffer.from('test'),
      printer: 'InvalidPrinterName123',
      docname: 'Error Test'
    });
    console.log('⚠️ Unexpected: Should have thrown error');
  } catch (error) {
    console.log(`✅ Correctly caught error: ${error.message}`);
  }

  // Test 2: Missing required parameters
  console.log('\n❌ Test 2: Missing required parameters');
  try {
    await jobs.printRaw({
      // Missing data and printer
      docname: 'Error Test'
    });
    console.log('⚠️ Unexpected: Should have thrown error');
  } catch (error) {
    console.log(`✅ Correctly caught error: ${error.message}`);
  }

  // Test 3: Invalid data type
  console.log('\n❌ Test 3: Invalid data type');
  try {
    const printerList = await printers.list();
    if (printerList.length > 0) {
      await jobs.printRaw({
        data: { invalid: 'object' }, // Should be Buffer or string
        printer: printerList[0].name,
        docname: 'Error Test'
      });
      console.log('⚠️ Unexpected: Should have thrown error');
    } else {
      console.log('⚠️ Skipped: No printers available');
    }
  } catch (error) {
    console.log(`✅ Correctly caught error: ${error.message}`);
  }

  // Test 5: Error types and properties
  console.log('\n🔍 Test 5: Error types and properties');
  try {
    console.log(`PrinterError available: ${typeof PrinterError === 'function'}`);

    // Test creating custom PrinterError
    const customError = new PrinterError('Test error', 'TEST_ERROR');
    console.log(`✅ Custom error created: ${customError.message} (code: ${customError.code})`);
  } catch (error) {
    console.error(`❌ Error type test failed: ${error.message}`);
  }

  // Test 6: Graceful degradation when no printers
  console.log('\n🔍 Test 6: Graceful degradation');
  try {
    const printerList = await printers.list();

    if (printerList.length === 0) {
      console.log('✅ No printers found - library handles gracefully');

      try {
        await printers.default();
        console.log('⚠️ Unexpected: Should have thrown error for no default printer');
      } catch (error) {
        console.log(`✅ Correctly handles no default printer: ${error.message}`);
      }
    } else {
      console.log(`✅ Found ${printerList.length} printers - system is ready`);
    }
  } catch (error) {
    console.error(`❌ Printer listing failed: ${error.message}`);
  }

  console.log('\n✅ Error handling tests completed!');
}

if (require.main === module) {
  testErrorHandling().catch(console.error);
}
