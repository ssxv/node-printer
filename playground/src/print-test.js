#!/usr/bin/env node
/**
 * Print test - Test actual printing functionality
 * Run with: node print-test.js
 */

const path = require('path');

console.log('=== Print Test ===\n');

const { jobs } = require('@ssxv/node-printer');
const legacy = require('@ssxv/node-printer/printer');
const testFile = path.join(__dirname, 'test-files', 'sample.txt');

// Create test content
const testContent = `Hello from ssxv/node-printer playground!

This is a test document generated at: ${new Date().toISOString()}

Testing basic printing functionality:
- Print direct text
- Print from buffer
- Print with options

End of test document.`;

const printerName = 'Satyendra-local-receipt-printer';

async function testPrinting() {
  console.log('\n📄 Test 1: Print direct text raw');
  try {
    const job = await jobs.printRaw({
      printer: printerName,
      data: Buffer.from(testContent, 'utf8')
    });
    console.log(`✅ Print job submitted successfully! Job info:`, job);
  } catch (error) {
    console.error('❌ Modern API print failed:', error.message);
  }

  console.log('\n📄 Test 2: Print file');
  try {
    const job = await jobs.printFile({
      printer: printerName,
      file: testFile
    });
    console.log(`✅ Print job submitted successfully! Job info:`, job);
  } catch (error) {
    console.error('❌ Modern API print failed:', error.message);
  }

  console.log('\n📄 Test 3: Print direct text raw (legacy)');
  try {
    const job = await legacy.printDirect({
      printer: printerName,
      data: testContent
    });
    console.log(`✅ Legacy print job submitted successfully!`, job);
  } catch (error) {
    console.error('❌ Legacy API print failed:', error.message);
  }

  console.log('\n📄 Test 4: Print from file (legacy)');
  try {
    const job = await legacy.printFile({
      printer: printerName,
      filename: testFile
    });
    console.log(`✅ File print job submitted successfully!`, job);
  } catch (error) {
    console.error('❌ File print failed:', error.message);
  }
}

if (require.main === module) {
  testPrinting().catch(console.error);
}
