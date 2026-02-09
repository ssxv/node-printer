#!/usr/bin/env node
/**
 * Print test - Test actual printing functionality
 * Run with: node print-test.js
 */

const fs = require('fs');
const path = require('path');

console.log('=== Print Test ===\n');

// Create test content
const testContent = `Hello from node-printer playground!

This is a test document generated at: ${new Date().toISOString()}

Testing basic printing functionality:
- Print direct text
- Print from buffer
- Print with options

End of test document.`;

async function testPrinting() {
  try {
    const { printers, jobs } = require('@ssxv/node-printer');
    
    // Get available printers
    const printerList = await printers.list();
    if (printerList.length === 0) {
      console.log('❌ No printers found. Cannot test printing.');
      return;
    }

    // Try to get default printer, otherwise use first available
    let targetPrinter;
    try {
      targetPrinter = await printers.default();
      console.log(`🖨️  Using default printer: ${targetPrinter.name}`);
    } catch (e) {
      targetPrinter = printerList[0];
      console.log(`🖨️  Using first available printer: ${targetPrinter.name}`);
    }

    // Test 1: Print direct text (modern API)
    console.log('\n📄 Test 1: Print direct text (modern API)');
    try {
      const job = await jobs.printRaw({
        data: Buffer.from(testContent, 'utf8'),
        printer: targetPrinter.name,
        docname: 'Playground Test - Modern API',
        type: 'RAW'
      });
      console.log(`✅ Print job submitted successfully! Job info:`, job);
    } catch (error) {
      console.error('❌ Modern API print failed:', error.message);
    }

    // Test 2: Print using legacy API
    console.log('\n📄 Test 2: Print using legacy API');
    try {
      const printer = require('@ssxv/node-printer/printer');
      const job = await printer.printDirect({
        data: testContent,
        printer: targetPrinter.name,
        docname: 'Playground Test - Legacy API',
        type: 'RAW'
      });
      console.log(`✅ Legacy print job submitted successfully!`, job);
    } catch (error) {
      console.error('❌ Legacy API print failed:', error.message);
    }

    // Test 3: Print test file
    console.log('\n📄 Test 3: Print from file');
    const testFile = path.join(__dirname, 'test-files', 'sample.txt');
    
    // Create test file if it doesn't exist
    if (!fs.existsSync(testFile)) {
      fs.writeFileSync(testFile, testContent);
      console.log(`📝 Created test file: ${testFile}`);
    }

    try {
      const printer = require('@ssxv/node-printer/printer');
      const job = await printer.printFile({
        filename: testFile,
        printer: targetPrinter.name,
        docname: 'Playground File Print Test'
      });
      console.log(`✅ File print job submitted successfully!`, job);
    } catch (error) {
      console.error('❌ File print failed:', error.message);
      console.log('💡 Note: printFile may not be available on all platforms');
    }

  } catch (error) {
    console.error('❌ Print test failed:', error.message);
    console.error(error.stack);
  }
}

if (require.main === module) {
  console.log('⚠️  WARNING: This will attempt to print to your default printer!');
  console.log('⚠️  Make sure your printer is ready or cancel within 5 seconds...\n');
  
  setTimeout(() => {
    testPrinting().catch(console.error);
  }, 5000);
}