#!/usr/bin/env node
/**
 * Jobs test - Test actual job management functionality
 * Run with: node jobs-test.js
 */

console.log('=== Jobs Test ===\n');

const { printers, jobs } = require('@ssxv/node-printer');

async function testPrinting() {
  const printerName = (await printers.default()).name;
  console.log('\n📄 Test 1: Get jobs');
  let jobList;
  try {
    jobList = await jobs.list();
    console.log(`✅ Print jobs:`, jobList);
  } catch (error) {
    console.error('❌ Modern API print failed:', error.message);
  }

  // trim jobList to keep just last 4 jobs for testing
  jobList = jobList.slice(-4);

  console.log('\n📄 Test 2: Get jobs with options');
  try {
    const jobList = await jobs.list({ printer: printerName });
    console.log(`✅ Print jobs:`, jobList);
  } catch (error) {
    console.error('❌ Modern API print failed:', error.message);
  }

  console.log('\n📄 Test 3: Get job details');
  try {
    const jobDetail = await jobs.get(printerName, jobList[0].id);
    console.log(`✅ Print jobs:`, jobDetail);
  } catch (error) {
    console.error('❌ Modern API print failed:', error.message);
  }

  console.log('\n📄 Test 4: Pause job with native command');
  try {
    await jobs.setNative(printerName, jobList[1].id, 'pause');
    const jobDetail = await jobs.get(printerName, jobList[1].id);
    console.log(`✅ Print jobs:`, jobDetail);
  } catch (error) {
    console.error('❌ Modern API print failed:', error.message);
  }

  console.log('\n📄 Test 5: Resume job with native command');
  try {
    await jobs.setNative(printerName, jobList[1].id, 'resume');
    const jobDetail = await jobs.get(printerName, jobList[1].id);
    console.log(`✅ Print jobs:`, jobDetail);
  } catch (error) {
    console.error('❌ Modern API print failed:', error.message);
  }

  console.log('\n📄 Test 6: Cancel job with native command');
  try {
    await jobs.setNative(printerName, jobList[2].id, 'cancel');
    const jobDetail = await jobs.get(printerName, jobList[2].id);
    console.log(`✅ Print jobs:`, jobDetail);
  } catch (error) {
    console.error('❌ Modern API print failed:', error.message);
  }

  console.log('\n📄 Test 7: Cancel job');
  try {
    await jobs.cancel(printerName, jobList[3].id);
    const jobDetail = await jobs.get(printerName, jobList[3].id);
    console.log(`✅ Print jobs:`, jobDetail);
  } catch (error) {
    console.error('❌ Modern API print failed:', error.message);
  }
}

if (require.main === module) {
  testPrinting().catch(console.error);
}
