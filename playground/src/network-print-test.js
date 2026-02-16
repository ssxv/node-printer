#!/usr/bin/env node
/**
 * Network Print test - Test printSocket functionality for direct network printing
 * Run with: node network-print-test.js
 *
 * NOTE: Change the printer IP address below to match your network printer
 */

console.log('=== Network Print Test ===\n');

const { network } = require('@ssxv/node-printer');

// CHANGE THIS: Set to your actual network printer IP
const printerIP = '192.168.1.23';
const printerPort = 9100; // Standard JetDirect RAW port

// ESC/POS commands for receipt printers
const escPosCommands = {
  // Initialize printer
  init: Buffer.from([0x1b, 0x40]),
  // Print text and line feed
  text: Buffer.from('Hello from network printing!\nTest document\n'),
  // Cut paper (if supported)
  cut: Buffer.from([0x1d, 0x56, 0x00])
};

// Combine commands for a complete test print
const testDocument = Buffer.concat([
  escPosCommands.init,
  Buffer.from(`Network Print Test\n`),
  Buffer.from(`Time: ${new Date().toISOString()}\n`),
  Buffer.from(`Printer: ${printerIP}:${printerPort}\n`),
  Buffer.from(`Test completed successfully!\n`),
  Buffer.from(`\n\n\n\n`), // Extra spacing to ensure all text prints before cut
  escPosCommands.cut
]);

async function testNetworkPrinting() {
  console.log(`🖨️ Testing network printer at ${printerIP}:${printerPort}\n`);

  console.log('📄 Test 1: Fire-and-forget printing (no response)');
  try {
    await network.printSocket({
      host: printerIP,
      port: printerPort,
      data: testDocument,
      retries: 2
    });
    console.log(`✅ Print sent successfully to ${printerIP}:${printerPort}`);
    console.log('   (No response expected in fire-and-forget mode)');
  } catch (error) {
    console.error('❌ Fire-and-forget print failed:', error.message);
    console.error(`   Error code: ${error.code || 'UNKNOWN'}`);
  }

  console.log('\n📄 Test 2: Simple text printing');
  try {
    const simpleText = Buffer.from('Simple network print test\n\n');
    await network.printSocket({
      host: printerIP,
      data: simpleText
    });
    console.log(`✅ Simple text sent to ${printerIP}:9100`);
  } catch (error) {
    console.error('❌ Simple text print failed:', error.message);
    console.error(`   Error code: ${error.code || 'UNKNOWN'}`);
  }

  console.log('\n📄 Test 3: Error handling (invalid IP)');
  try {
    await network.printSocket({
      host: '192.168.255.255', // Likely invalid IP
      port: 9100,
      data: Buffer.from('This should fail\n'),
      timeout: 2000,
      retries: 1
    });
    console.log(`⚠️ Unexpected success with 192.168.255.255:9100`);
  } catch (error) {
    console.log('✅ Error handling working correctly:');
    console.log(`   Error: ${error.message}`);
    console.log(`   Code: ${error.code || 'UNKNOWN'}`);
  }

  console.log('\n📄 Test 4: Connection timeout test');
  try {
    await network.printSocket({
      host: '10.0.0.1', // Likely unreachable IP
      port: 9100,
      data: Buffer.from('Timeout test\n'),
      timeout: 1000, // Short timeout
      retries: 0
    });
    console.log(`⚠️ Unexpected success with 10.0.0.1:9100`);
  } catch (error) {
    console.log('✅ Timeout handling working correctly:');
    console.log(`   Error: ${error.message}`);
    console.log(`   Code: ${error.code || 'UNKNOWN'}`);
  }

  console.log('\nNetwork printing tests completed!');
  console.log('\nTips:');
  console.log('- Update the printerIP variable to match your network printer');
  console.log('- Most receipt printers use port 9100');
  console.log('- Use fire-and-forget mode (no timeout) for simple printing');
  console.log('- Use timeout mode for better error handling on unreliable networks');
  console.log('- ESC/POS printers respond to the commands used here');
}

testNetworkPrinting().catch(console.error);
