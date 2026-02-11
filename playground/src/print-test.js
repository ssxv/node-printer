#!/usr/bin/env node
/**
 * Print test - Test actual printing functionality
 * Run with: node print-test.js
 */

const path = require('path');

console.log('=== Print Test ===\n');

const { jobs } = require('@ssxv/node-printer');
const sampleTxtFile = path.join(__dirname, 'test-files', 'sample.txt');
const samplePdfFile = path.join(__dirname, 'test-files', 'sample.pdf');
const sampleImageFile = path.join(__dirname, 'test-files', 'sample.jpg');

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

  console.log('\n📄 Test 2: Print text file');
  try {
    const job = await jobs.printFile({
      printer: printerName,
      file: sampleTxtFile
    });
    console.log(`✅ Print job submitted successfully! Job info:`, job);
  } catch (error) {
    console.error('❌ Modern API print failed:', error.message);
  }

  console.log('\n📄 Test 3: Print Pdf file');
  try {
    const job = await jobs.printFile({
      printer: printerName,
      file: samplePdfFile
    });
    console.log(`✅ Print job submitted successfully! Job info:`, job);
  } catch (error) {
    console.error('❌ Modern API print failed:', error.message);
  }

  console.log('\n📄 Test 4: Print image file');
  try {
    const job = await jobs.printFile({
      printer: printerName,
      file: sampleImageFile
    });
    console.log(`✅ Print job submitted successfully! Job info:`, job);
  } catch (error) {
    console.error('❌ Modern API print failed:', error.message);
  }
}

if (require.main === module) {
  testPrinting().catch(console.error);
}
