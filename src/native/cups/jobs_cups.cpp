// CUPS-specific job implementation
// Implements IJobAPI using CUPS API

#include "../job_api.h"
#include "../errors.h"
#include "../../mapping/job_state.h"
#include <cups/cups.h>
#include <vector>
#include <string>
#include <map>
#include <mutex>
#include <fstream>
#include <unistd.h>
#include <fcntl.h>

namespace NodePrinter {

// CUPS is not thread-safe, so we need a global mutex
extern std::mutex g_cupsMutex;

// Options management class (from original implementation)
class CupsOptionsManager {
private:
  cups_option_t* options;
  int num_options;

public:
  CupsOptionsManager() : options(nullptr), num_options(0) {}
  
  CupsOptionsManager(const PrintOptions& printOptions) : options(nullptr), num_options(0) {
    // Convert PrintOptions to CUPS options
    if (printOptions.copies > 1) {
      num_options = cupsAddOption("copies", std::to_string(printOptions.copies).c_str(), 
                                  num_options, &options);
    }
    
    if (printOptions.duplex) {
      num_options = cupsAddOption("sides", "two-sided-long-edge", num_options, &options);
    }
    
    if (printOptions.color) {
      num_options = cupsAddOption("ColorModel", "RGB", num_options, &options);
      num_options = cupsAddOption("print-color-mode", "color", num_options, &options);
    } else {
      num_options = cupsAddOption("ColorModel", "Gray", num_options, &options);
      num_options = cupsAddOption("print-color-mode", "monochrome", num_options, &options);
    }
    
    if (!printOptions.paperSize.empty()) {
      num_options = cupsAddOption("PageSize", printOptions.paperSize.c_str(), num_options, &options);
      num_options = cupsAddOption("media", printOptions.paperSize.c_str(), num_options, &options);
    }
    
    if (printOptions.orientation == "landscape") {
      num_options = cupsAddOption("orientation-requested", "4", num_options, &options);
    } else if (printOptions.orientation == "portrait") {
      num_options = cupsAddOption("orientation-requested", "3", num_options, &options);
    }
  }
  
  ~CupsOptionsManager() {
    if (options) {
      cupsFreeOptions(num_options, options);
    }
  }
  
  // Non-copyable
  CupsOptionsManager(const CupsOptionsManager&) = delete;
  CupsOptionsManager& operator=(const CupsOptionsManager&) = delete;
  
  // Move constructor/assignment
  CupsOptionsManager(CupsOptionsManager&& other) noexcept 
    : options(other.options), num_options(other.num_options) {
    other.options = nullptr;
    other.num_options = 0;
  }
  
  CupsOptionsManager& operator=(CupsOptionsManager&& other) noexcept {
    if (this != &other) {
      if (options) cupsFreeOptions(num_options, options);
      options = other.options;
      num_options = other.num_options;
      other.options = nullptr;
      other.num_options = 0;
    }
    return *this;
  }
  
  cups_option_t* get() { return options; }
  int getNumOptions() const { return num_options; }
};

class CupsJobAPI : public IJobAPI {
private:
  // Threshold for using temporary files
  static const size_t STREAM_THRESHOLD = 4 * 1024 * 1024; // 4 MiB
  
  // Convert format string to CUPS format
  std::string formatToCups(const std::string& format) {
    static const std::map<std::string, std::string> formatMap = {
      {"RAW", CUPS_FORMAT_RAW},
      {"TEXT", CUPS_FORMAT_TEXT},
#ifdef CUPS_FORMAT_PDF
      {"PDF", CUPS_FORMAT_PDF},
#endif
#ifdef CUPS_FORMAT_JPEG
      {"JPEG", CUPS_FORMAT_JPEG},
      {"IMAGE", CUPS_FORMAT_JPEG},
#endif 
#ifdef CUPS_FORMAT_POSTSCRIPT
      {"POSTSCRIPT", CUPS_FORMAT_POSTSCRIPT},
#endif
#ifdef CUPS_FORMAT_AUTO
      {"AUTO", CUPS_FORMAT_AUTO},
#endif
    };
    
    auto it = formatMap.find(format);
    return (it != formatMap.end()) ? it->second : CUPS_FORMAT_RAW;
  }
  
public:
  int printFile(const PrintFileRequest& request) override {
    std::lock_guard<std::mutex> lock(g_cupsMutex);
    
    // Validate file exists
    std::ifstream file(request.filename);
    if (!file.is_open()) {
      throw createFileNotFoundError(request.filename);
    }
    file.close();
    
    CupsOptionsManager options(request.options);
    
    std::string jobName = request.options.jobName.empty() ? "Node.js Print Job" : request.options.jobName;
    
    int jobId = cupsPrintFile(request.printer.c_str(), request.filename.c_str(), 
                              jobName.c_str(), options.getNumOptions(), options.get());
    
    if (jobId == 0) {
      throw ErrorMappers::createCupsError("CUPS print failed");
    }
    
    return jobId;
  }
  
  int printRaw(const PrintRawRequest& request) override {
    std::lock_guard<std::mutex> lock(g_cupsMutex);
    
    CupsOptionsManager options(request.options);
    std::string jobName = request.options.jobName.empty() ? "Node.js Print Job" : request.options.jobName;
    
    int jobId = 0;
    
    if (request.data.size() > STREAM_THRESHOLD) {
      // Use temporary file for large data
      char tempPath[] = "/tmp/nodeprinter_XXXXXX";
      int tempFd = mkstemp(tempPath);
      if (tempFd == -1) {
        throw ErrorMappers::createCupsError("Failed to create temporary file");
      }
      
      // Write data to temp file
      ssize_t written = write(tempFd, request.data.data(), request.data.size());
      close(tempFd);
      
      if (written != static_cast<ssize_t>(request.data.size())) {
        unlink(tempPath);
        throw ErrorMappers::createCupsError("Failed to write data to temporary file");
      }
      
      // Print using temp file
      jobId = cupsPrintFile(request.printer.c_str(), tempPath, jobName.c_str(), 
                            options.getNumOptions(), options.get());
      
      // Clean up temp file
      unlink(tempPath);
    } else {
      // Use temporary file for small data too (simpler and more reliable)
      char tempPath[] = "/tmp/nodeprinter_small_XXXXXX";
      int tempFd = mkstemp(tempPath);
      if (tempFd == -1) {
        throw ErrorMappers::createCupsError("Failed to create temporary file for small data");
      }
      
      // Write data to temp file
      ssize_t written = write(tempFd, request.data.data(), request.data.size());
      close(tempFd);
      
      if (written != static_cast<ssize_t>(request.data.size())) {
        unlink(tempPath);
        throw ErrorMappers::createCupsError("Failed to write small data to temporary file");
      }
      
      // Print using temp file
      jobId = cupsPrintFile(request.printer.c_str(), tempPath, jobName.c_str(), 
                            options.getNumOptions(), options.get());
      
      // Clean up temp file
      unlink(tempPath);
    }
    
    if (jobId == 0) {
      throw ErrorMappers::createCupsError("CUPS print failed");
    }
    
    return jobId;
  }
  
  JobInfo getJob(const std::string& printer, int jobId) override {
    std::lock_guard<std::mutex> lock(g_cupsMutex);
    
    cups_job_t* jobs = nullptr;
    int numJobs = cupsGetJobs(&jobs, printer.c_str(), 0, CUPS_WHICHJOBS_ALL);
    
    if (numJobs < 0) {
      throw ErrorMappers::createCupsError("Failed to get jobs from CUPS");
    }
    
    JobInfo info;
    bool found = false;
    
    for (int i = 0; i < numJobs; ++i) {
      if (jobs[i].id == jobId) {
        info.id = jobId;
        info.state = JobMapping::mapCupsJobState(jobs[i].state);
        info.printer = printer;
        
        if (jobs[i].title) {
          info.title = jobs[i].title;
        }
        
        if (jobs[i].user) {
          info.user = jobs[i].user;
        }
        
        info.size = jobs[i].size;
        info.creationTime = static_cast<int64_t>(jobs[i].creation_time);
        info.processingTime = static_cast<int64_t>(jobs[i].processing_time);
        info.completedTime = static_cast<int64_t>(jobs[i].completed_time);
        
        found = true;
        break;
      }
    }
    
    cupsFreeJobs(numJobs, jobs);
    
    if (!found) {
      throw createJobNotFoundError(jobId);
    }
    
    return info;
  }
  
  std::vector<JobInfo> getJobs(const std::string& printer) override {
    std::lock_guard<std::mutex> lock(g_cupsMutex);
    
    cups_job_t* jobs = nullptr;
    const char* printerName = printer.empty() ? nullptr : printer.c_str();
    int numJobs = cupsGetJobs(&jobs, printerName, 0, CUPS_WHICHJOBS_ALL);
    
    std::vector<JobInfo> result;
    
    if (numJobs < 0) {
      return result; // Return empty list on error
    }
    
    for (int i = 0; i < numJobs; ++i) {
      JobInfo info;
      info.id = jobs[i].id;
      info.state = JobMapping::mapCupsJobState(jobs[i].state);
      
      if (jobs[i].dest) {
        info.printer = jobs[i].dest;
      } else if (!printer.empty()) {
        info.printer = printer;
      }
      
      if (jobs[i].title) {
        info.title = jobs[i].title;
      }
      
      if (jobs[i].user) {
        info.user = jobs[i].user;
      }
      
      info.size = jobs[i].size;
      info.creationTime = static_cast<int64_t>(jobs[i].creation_time);
      info.processingTime = static_cast<int64_t>(jobs[i].processing_time);  
      info.completedTime = static_cast<int64_t>(jobs[i].completed_time);
      
      result.push_back(std::move(info));
    }
    
    cupsFreeJobs(numJobs, jobs);
    return result;
  }
  
  void setJob(const std::string& printer, int jobId, JobCommand command) override {
    std::lock_guard<std::mutex> lock(g_cupsMutex);
    
    int result = 0;
    
    switch (command) {
      case JobCommand::CANCEL:
        result = cupsCancelJob(printer.c_str(), jobId);
        break;
      case JobCommand::PAUSE:
      case JobCommand::RESUME:
        // CUPS doesn't have direct pause/resume - these would require IPP operations
        throw PrinterException("Pause/Resume not supported in CUPS implementation", PrinterErrorCode::UNSUPPORTED_FORMAT);
      default:
        throw createInvalidArgumentsError("Unknown job command");
    }
    
    if (result != 1) {
      throw ErrorMappers::createCupsError("CUPS job control failed");
    }
  }
};

} // namespace NodePrinter