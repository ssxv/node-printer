#!/usr/bin/env node
/**
 * Jobs test - Test actual job management functionality
 * Run with: node jobs-test.js
 */

console.log("=== Jobs Test ===\n");

const { jobs } = require("@ssxv/node-printer");

const printerName = "Satyendra-local-receipt-printer";
const jobId = 80; // Replace with an actual job ID from your printer

async function testPrinting() {
  console.log("\n📄 Test 1: Get jobs");
  try {
    const jobList = await jobs.list();
    console.log(`✅ Print jobs:`, jobList);
  } catch (error) {
    console.error("❌ Modern API print failed:", error.message);
  }

  console.log("\n📄 Test 2: Get jobs with options");
  try {
    const jobList = await jobs.list({ printer: printerName });
    console.log(`✅ Print jobs:`, jobList);
  } catch (error) {
    console.error("❌ Modern API print failed:", error.message);
  }

  console.log("\n📄 Test 3: Get job details");
  try {
    const jobDetail = await jobs.get(printerName, jobId);
    console.log(`✅ Print jobs:`, jobDetail);
  } catch (error) {
    console.error("❌ Modern API print failed:", error.message);
  }

  console.log("\n📄 Test 4: Pause job with native command");
  try {
    await jobs.setNative(printerName, jobId, "pause");
    const jobDetail = await jobs.get(printerName, jobId);
    console.log(`✅ Print jobs:`, jobDetail);
  } catch (error) {
    console.error("❌ Modern API print failed:", error.message);
  }

  console.log("\n📄 Test 5: Resume job with native command");
  try {
    await jobs.setNative(printerName, jobId, "resume");
    const jobDetail = await jobs.get(printerName, jobId);
    console.log(`✅ Print jobs:`, jobDetail);
  } catch (error) {
    console.error("❌ Modern API print failed:", error.message);
  }

  console.log("\n📄 Test 6: Cancel job with native command");
  try {
    await jobs.setNative(printerName, jobId, "cancel");
    const jobDetail = await jobs.get(printerName, jobId);
    console.log(`✅ Print jobs:`, jobDetail);
  } catch (error) {
    console.error("❌ Modern API print failed:", error.message);
  }

  console.log("\n📄 Test 7: Cancel job");
  try {
    await jobs.cancel(printerName, jobId);
    const jobDetail = await jobs.get(printerName, jobId);
    console.log(`✅ Print jobs:`, jobDetail);
  } catch (error) {
    console.error("❌ Modern API print failed:", error.message);
  }
}

if (require.main === module) {
  testPrinting().catch(console.error);
}
