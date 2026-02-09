#pragma once
#include <napi.h>
#include <vector>
#include <string>
#include <memory>

namespace NodePrinter {

/**
 * Cross-platform job information
 * All OS-specific data is normalized to this structure
 */
struct JobInfo {
    int id;
    std::string state;        // normalized: "pending", "printing", "completed", "canceled", "error"
    std::string printer;
    std::string title;
    std::string user;
    int64_t creationTime = 0;     // Unix timestamp
    int64_t processingTime = 0;   // Unix timestamp  
    int64_t completedTime = 0;    // Unix timestamp
    int pages = 0;
    int64_t size = 0;             // Size in bytes
};

/**
 * Print job options (normalized across platforms)
 */
struct PrintOptions {
    int copies = 1;
    bool duplex = false;
    bool color = false;
    std::string paperSize;
    std::string orientation;      // "portrait" or "landscape"
    std::string jobName;
};

/**
 * Print file request parameters
 */
struct PrintFileRequest {
    std::string printer;
    std::string filename;
    PrintOptions options;
};

/**
 * Print raw data request parameters
 */
struct PrintRawRequest {
    std::string printer;
    std::vector<uint8_t> data;
    std::string format;           // "RAW", "TEXT", etc.
    PrintOptions options;
};

/**
 * Job control commands
 */
enum class JobCommand {
    PAUSE,
    RESUME, 
    CANCEL
};

/**
 * Abstract job API interface
 * Platform-specific implementations must inherit from this
 */
class IJobAPI {
public:
    virtual ~IJobAPI() = default;
    
    /**
     * Print a file
     * @param request Print file request parameters
     * @returns Job ID
     */
    virtual int printFile(const PrintFileRequest& request) = 0;
    
    /**
     * Print raw data
     * @param request Print raw data request parameters
     * @returns Job ID
     */
    virtual int printRaw(const PrintRawRequest& request) = 0;
    
    /**
     * Get information about a specific job
     * @param printer Printer name
     * @param jobId Job ID
     */
    virtual JobInfo getJob(const std::string& printer, int jobId) = 0;
    
    /**
     * Get list of jobs for a printer
     * @param printer Printer name (empty string for all printers)
     */
    virtual std::vector<JobInfo> getJobs(const std::string& printer = "") = 0;
    
    /**
     * Control a job (pause, resume, cancel)
     * @param printer Printer name
     * @param jobId Job ID
     * @param command Command to execute
     */
    virtual void setJob(const std::string& printer, int jobId, JobCommand command) = 0;
};

/**
 * Factory function to create platform-specific job API
 */
std::unique_ptr<IJobAPI> createJobAPI();

/**
 * Utility functions for state and option mapping
 */
namespace JobMapping {
    std::string mapJobState(uint32_t winStatus);
    std::string mapCupsJobState(int cupsState);
    JobCommand parseCommand(const std::string& commandStr);
}

} // namespace NodePrinter