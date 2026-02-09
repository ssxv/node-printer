#!/usr/bin/env node
/**
 * Printer explorer - Detailed information about available printers
 * Run with: node printer-explorer.js
 */

console.log('=== Printer Explorer ===\n');

async function explorePrinters() {
  try {
    const { printers } = require('@ssxv/node-printer');
    const printer = require('@ssxv/node-printer/printer');

    console.log('🔍 Exploring available printers in detail...\n');

    // Get printer list
    const printerList = await printers.list();
    console.log(`Found ${printerList.length} printers:\n`);

    if (printerList.length === 0) {
      console.log('❌ No printers found. Please check your printer installation.');
      return;
    }

    // Explore each printer
    for (let i = 0; i < printerList.length; i++) {
      const p = printerList[i];
      console.log(`🖨️  Printer ${i + 1}: ${p.name}`);
      console.log(`   └─ Default: ${p.isDefault ? '✅ Yes' : '❌ No'}`);
      console.log(`   └─ State: ${p.state}`);
      if (p.location) console.log(`   └─ Location: ${p.location}`);
      if (p.description) console.log(`   └─ Description: ${p.description}`);
      
      // Try to get detailed information via legacy API
      try {
        const detailed = await printer.getPrinter(p.name);
        console.log(`   └─ Detailed info available: ✅`);
        
        if (detailed.status && Array.isArray(detailed.status)) {
          console.log(`   └─ Raw status: [${detailed.status.join(', ')}]`);
        } else if (detailed.status) {
          console.log(`   └─ Raw status: ${detailed.status}`);
        }
        
        if (detailed.jobs && Array.isArray(detailed.jobs)) {
          console.log(`   └─ Active jobs: ${detailed.jobs.length}`);
          if (detailed.jobs.length > 0) {
            detailed.jobs.slice(0, 3).forEach((job, idx) => {
              console.log(`      └─ Job ${idx + 1}: ${job.name || job.document || 'Unknown'} (${job.status})`);
            });
            if (detailed.jobs.length > 3) {
              console.log(`      └─ ... and ${detailed.jobs.length - 3} more`);
            }
          }
        }
        
        // Show other properties
        Object.keys(detailed).forEach(key => {
          if (!['name', 'status', 'jobs', 'isDefault'].includes(key) && detailed[key] !== undefined) {
            let value = detailed[key];
            if (typeof value === 'object' && value !== null) {
              value = JSON.stringify(value);
            }
            console.log(`   └─ ${key}: ${value}`);
          }
        });
        
      } catch (error) {
        console.log(`   └─ Detailed info: ❌ ${error.message}`);
      }
      
      console.log(''); // Empty line between printers
    }

    // Test default printer functionality
    console.log('🎯 Default printer analysis:');
    try {
      const defaultPrinter = await printers.default();
      console.log(`✅ Default printer found: ${defaultPrinter.name}`);
      
      // Compare with system default
      try {
        const systemDefault = await printer.getPrinters();
        const systemDefaultName = systemDefault.find(p => p.isDefault)?.name;
        if (systemDefaultName === defaultPrinter.name) {
          console.log('✅ Default printer matches system default');
        } else {
          console.log(`⚠️  Different defaults: API reports '${defaultPrinter.name}', system shows '${systemDefaultName}'`);
        }
      } catch (e) {
        console.log('⚠️  Could not compare with system default');
      }
      
    } catch (error) {
      console.log(`❌ No default printer: ${error.message}`);
    }

    // Platform-specific information
    console.log('\n💻 Platform information:');
    console.log(`   └─ OS: ${process.platform} ${process.arch}`);
    console.log(`   └─ Node.js: ${process.version}`);
    
    // Check for common printer types
    console.log('\n📊 Printer type analysis:');
    const printerNames = printerList.map(p => p.name.toLowerCase());
    const types = {
      pdf: printerNames.filter(n => n.includes('pdf')).length,
      fax: printerNames.filter(n => n.includes('fax')).length,
      network: printerNames.filter(n => n.includes('network') || n.includes('ip') || n.includes('\\\\')).length,
      local: printerNames.filter(n => n.includes('usb') || n.includes('local')).length
    };
    
    Object.entries(types).forEach(([type, count]) => {
      if (count > 0) {
        console.log(`   └─ ${type.toUpperCase()}: ${count} printer(s)`);
      }
    });

    console.log('\n✅ Printer exploration completed!');
    console.log('\n💡 Use this information to choose the right printer for your print tests.');

  } catch (error) {
    console.error('❌ Printer exploration failed:', error.message);
    console.error(error.stack);
  }
}

if (require.main === module) {
  explorePrinters().catch(console.error);
}