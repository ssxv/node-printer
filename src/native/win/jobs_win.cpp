// Windows-specific job implementation
// Implements IJobAPI using Winspool API

#include "../job_api.h"
#include "../errors.h"
#include "../../mapping/job_state.h"
#include "win_utils.h"
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

namespace NodePrinter {

using namespace WinUtils;

class WinJobAPI : public IJobAPI {
public:
  int printFile(const PrintFileRequest& request) override {
    std::wstring printerName = WinUtils::utf8_to_ws(request.printer);
    WinUtils::PrinterHandle handle(printerName.c_str());
    
    if (!handle.isOk()) {
      throw ErrorMappers::createWindowsError("Failed to open printer: " + request.printer);
    }
    
    // Read file content
    std::ifstream file(request.filename, std::ios::binary);
    if (!file.is_open()) {
      throw createFileNotFoundError(request.filename);
    }
    
    // Get file size
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    
    // Read content
    std::vector<char> content(fileSize);
    file.read(content.data(), fileSize);
    file.close();
    
    // Start print job
    std::wstring jobName = WinUtils::utf8_to_ws(request.options.jobName.empty() ? "Node.js Print Job" : request.options.jobName);
    
    DOC_INFO_1W docInfo;
    docInfo.pDocName = const_cast<LPWSTR>(jobName.c_str());
    docInfo.pOutputFile = NULL; 
    docInfo.pDatatype = const_cast<LPWSTR>(L"RAW"); // Default to RAW format
    
    DWORD jobId = StartDocPrinterW(handle, 1, reinterpret_cast<LPBYTE>(&docInfo));
    if (jobId == 0) {
      throw ErrorMappers::createWindowsError("Failed to start print job");
    }
    
    // Start page  
    if (!StartPagePrinter(handle)) {
      EndDocPrinter(handle);
      throw ErrorMappers::createWindowsError("Failed to start page");
    }
    
    // Write data
    DWORD bytesWritten = 0;
    if (!WritePrinter(handle, content.data(), static_cast<DWORD>(fileSize), &bytesWritten)) {
      EndPagePrinter(handle);
      EndDocPrinter(handle);
      throw ErrorMappers::createWindowsError("Failed to write to printer");
    }
    
    // End page and document
    EndPagePrinter(handle);
    EndDocPrinter(handle);
    
    return static_cast<int>(jobId);
  }
  
  int printRaw(const PrintRawRequest& request) override {
    std::wstring printerName = WinUtils::utf8_to_ws(request.printer);
    WinUtils::PrinterHandle handle(printerName.c_str());
    
    if (!handle.isOk()) {
      throw ErrorMappers::createWindowsError("Failed to open printer: " + request.printer);
    }
    
    // Start print job
    std::wstring jobName = WinUtils::utf8_to_ws(request.options.jobName.empty() ? "Node.js Print Job" : request.options.jobName);
    std::wstring dataType = WinUtils::utf8_to_ws(request.format.empty() ? "RAW" : request.format);
    
    DOC_INFO_1W docInfo;
    docInfo.pDocName = const_cast<LPWSTR>(jobName.c_str());
    docInfo.pOutputFile = NULL;
    docInfo.pDatatype = const_cast<LPWSTR>(dataType.c_str());
    
    DWORD jobId = StartDocPrinterW(handle, 1, reinterpret_cast<LPBYTE>(&docInfo));
    if (jobId == 0) {
      throw ErrorMappers::createWindowsError("Failed to start print job");
    }
    
    // Start page  
    if (!StartPagePrinter(handle)) {
      EndDocPrinter(handle);
      throw ErrorMappers::createWindowsError("Failed to start page");
    }
    
    // Write data
    DWORD bytesWritten = 0;
    if (!WritePrinter(handle, const_cast<uint8_t*>(request.data.data()), 
                      static_cast<DWORD>(request.data.size()), &bytesWritten)) {
      EndPagePrinter(handle);
      EndDocPrinter(handle);
      throw ErrorMappers::createWindowsError("Failed to write to printer");
    }
    
    // End page and document
    EndPagePrinter(handle);
    EndDocPrinter(handle);
    
    return static_cast<int>(jobId);
  }
  
  JobInfo getJob(const std::string& printer, int jobId) override {
    std::wstring printerName = WinUtils::utf8_to_ws(printer);
    WinUtils::PrinterHandle handle(printerName.c_str());
    
    if (!handle.isOk()) {
      throw createPrinterNotFoundError(printer);
    }
    
    DWORD needed = 0;
    GetJobW(handle, jobId, 2, NULL, 0, &needed);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
      throw createJobNotFoundError(jobId);
    }
    
    std::vector<BYTE> buffer(needed);
    JOB_INFO_2W* pJob = reinterpret_cast<JOB_INFO_2W*>(buffer.data());
    
    if (!GetJobW(handle, jobId, 2, buffer.data(), needed, &needed)) {
      throw ErrorMappers::createWindowsError("Failed to get job info");
    }
    
    JobInfo info;
    info.id = jobId;
    info.state = JobMapping::mapJobState(pJob->Status);
    info.printer = printer;
    
    if (pJob->pDocument) {
      info.title = WinUtils::ws_to_utf8(pJob->pDocument);
    }
    
    if (pJob->pUserName) {
      info.user = WinUtils::ws_to_utf8(pJob->pUserName);
    }
    
    info.pages = pJob->TotalPages;
    info.size = pJob->Size;
    
    // Convert Windows time fields (these are in local time)
    if (pJob->Submitted.wYear > 0) {
      info.creationTime = systemtime_to_unix_timestamp(pJob->Submitted);
    }
    
    return info;
  }
  
  std::vector<JobInfo> getJobs(const std::string& printer) override {
    std::vector<JobInfo> jobs;
    
    if (printer.empty()) {
      // Get jobs from all printers - not easily supported in Windows
      // For now, return empty list
      return jobs;
    }
    
    std::wstring printerName = WinUtils::utf8_to_ws(printer);
    WinUtils::PrinterHandle handle(printerName.c_str());
    
    if (!handle.isOk()) {
      throw createPrinterNotFoundError(printer);
    }
    
    DWORD needed = 0, returned = 0;
    EnumJobsW(handle, 0, 999999, 2, NULL, 0, &needed, &returned);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
      return jobs; // No jobs or error
    }
    
    std::vector<BYTE> buffer(needed);
    JOB_INFO_2W* pJobs = reinterpret_cast<JOB_INFO_2W*>(buffer.data());
    
    if (!EnumJobsW(handle, 0, 999999, 2, buffer.data(), needed, &needed, &returned)) {
      return jobs; // Error getting jobs
    }
    
    // Convert to JobInfo structs
    for (DWORD i = 0; i < returned; ++i) {
      JobInfo info;
      info.id = pJobs[i].JobId;
      info.state = JobMapping::mapJobState(pJobs[i].Status);
      info.printer = printer;
      
      if (pJobs[i].pDocument) {
        info.title = WinUtils::ws_to_utf8(pJobs[i].pDocument);
      }
      
      if (pJobs[i].pUserName) {
        info.user = WinUtils::ws_to_utf8(pJobs[i].pUserName);
      }
      
      info.pages = pJobs[i].TotalPages;
      info.size = pJobs[i].Size;
      
      if (pJobs[i].Submitted.wYear > 0) {
        info.creationTime = systemtime_to_unix_timestamp(pJobs[i].Submitted);
      }
      
      jobs.push_back(std::move(info));
    }
    
    return jobs;
  }
  
  void setJob(const std::string& printer, int jobId, JobCommand command) override {
    std::wstring printerName = WinUtils::utf8_to_ws(printer);
    WinUtils::PrinterHandle handle(printerName.c_str());
    
    if (!handle.isOk()) {
      throw createPrinterNotFoundError(printer);
    }
    
    DWORD winCommand = 0;
    switch (command) {
      case JobCommand::PAUSE:
        winCommand = JOB_CONTROL_PAUSE;
        break;
      case JobCommand::RESUME:
        winCommand = JOB_CONTROL_RESUME;
        break;
      case JobCommand::CANCEL:
        winCommand = JOB_CONTROL_CANCEL;
        break;
      default:
        throw createInvalidArgumentsError("Unknown job command");
    }
    
    if (!SetJobW(handle, jobId, 0, NULL, winCommand)) {
      throw ErrorMappers::createWindowsError("Failed to set job command");
    }
  }
};

} // namespace NodePrinter