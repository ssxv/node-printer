#!/usr/bin/env node
/**
 * Printer explorer - Detailed information about available printers
 * Run with: node printer-explorer.js
 */
const { printers } = require('@ssxv/node-printer');

const printerName = 'Satyendra-local-receipt-printer';

console.log('=== Printer Explorer ===\n');

async function explorePrinters() {
  console.log('🔍 Exploring printers...\n');

  console.log('\n📄 Test 1: List all printers');
  console.log(await printers.list());

  console.log('\n📄 Test 2: Get printer');
  console.log(await printers.get(printerName));

  console.log('\n📄 Test 3: Default printer');
  console.log(await printers.default());

  console.log('\n📄 Test 4: Printer capabilities');
  console.log(await printers.capabilities(printerName));

  console.log('\n📄 Test 5: Printer driverOptions');
  console.log(await printers.driverOptions(printerName));
}

if (require.main === module) {
  explorePrinters().catch(console.error);
}
