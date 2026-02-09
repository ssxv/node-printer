#!/usr/bin/env node
/**
 * Basic usage test - List available printers
 * Run with: node basic-usage.js
 */

console.log('=== Basic Usage Test ===\n');

async function testBasicUsage() {
  try {
    // Test modern API
    console.log('📋 Testing modern API...');
    const { printers } = require('@ssxv/node-printer');
    
    const printerList = await printers.list();
    console.log(`Found ${printerList.length} printers:\n`);
    
    printerList.forEach((printer, index) => {
      console.log(`${index + 1}. ${printer.name}`);
      console.log(`   Status: ${printer.state}`);
      console.log(`   Default: ${printer.isDefault ? 'Yes' : 'No'}`);
      if (printer.location) console.log(`   Location: ${printer.location}`);
      if (printer.description) console.log(`   Description: ${printer.description}`);
      console.log('');
    });

    // Test getting default printer
    try {
      const defaultPrinter = await printers.default();
      console.log(`🖨️  Default printer: ${defaultPrinter.name}`);
    } catch (e) {
      console.log('⚠️  No default printer found');
    }

    console.log('\n✅ Modern API test completed successfully!');

  } catch (error) {
    console.error('❌ Modern API test failed:', error.message);
  }

  try {
    // Test legacy API
    console.log('\n📋 Testing legacy API...');
    const printer = require('@ssxv/node-printer/printer');
    
    const legacyPrinters = await printer.getPrinters();
    console.log(`Legacy API found ${legacyPrinters.length} printers`);
    
    console.log('✅ Legacy API test completed successfully!');

  } catch (error) {
    console.error('❌ Legacy API test failed:', error.message);
  }
}

if (require.main === module) {
  testBasicUsage().catch(console.error);
}