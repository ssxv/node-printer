// Job state mapping utilities  
// Normalizes OS-specific job states to standard states

#pragma once
#include <string>
#include <algorithm>
#include <cctype>

#ifdef _WIN32
#include <windows.h>
#include <winspool.h>
#endif

#ifdef __linux__
#include <cups/cups.h>
#endif

namespace NodePrinter {
namespace JobMapping {

/**
 * Map Windows job status to normalized state
 */
std::string mapJobState(uint32_t winStatus) {
#ifdef _WIN32
    // Priority order: error > canceled > completed > printing > pending
    
    if (winStatus & (JOB_STATUS_ERROR | JOB_STATUS_BLOCKED_DEVQ | 
                     JOB_STATUS_USER_INTERVENTION)) {
        return "error";
    }
    
    if (winStatus & (JOB_STATUS_DELETING | JOB_STATUS_DELETED)) {
        return "canceled";  
    }
    
    if (winStatus & JOB_STATUS_PRINTED) {
        return "completed";
    }
    
    if (winStatus & (JOB_STATUS_PRINTING | JOB_STATUS_SPOOLING)) {
        return "printing";
    }
    
    if (winStatus & JOB_STATUS_PAUSED) {
        return "pending"; // Paused jobs are effectively pending
    }
    
    // Default to pending for jobs in queue
    return "pending";
#else
    // Fallback for non-Windows builds
    return "error";
#endif
}

/**
 * Map CUPS job state to normalized state
 */
std::string mapCupsJobState(int cupsState) {
#ifdef __linux__
    switch (cupsState) {
        case IPP_JOB_PENDING:
            return "pending";
        case IPP_JOB_HELD:
            return "pending"; // Held jobs are effectively pending
        case IPP_JOB_PROCESSING:
            return "printing";
        case IPP_JOB_STOPPED:
            return "error";   // Stopped usually indicates error
        case IPP_JOB_CANCELLED:
        case IPP_JOB_CANCELED:
            return "canceled";
        case IPP_JOB_ABORTED:
            return "error";
        case IPP_JOB_COMPLETED:
            return "completed";
        default:
            return "error";
    }
#else
    // Fallback for non-Linux builds
    return "error";
#endif
}

/**
 * Parse string command to JobCommand enum
 */
JobCommand parseCommand(const std::string& commandStr) {
    std::string upperCommand = commandStr;
    std::transform(upperCommand.begin(), upperCommand.end(), upperCommand.begin(), ::toupper);
    
    if (upperCommand == "PAUSE") {
        return JobCommand::PAUSE;
    } else if (upperCommand == "RESUME") {
        return JobCommand::RESUME;
    } else if (upperCommand == "CANCEL") {
        return JobCommand::CANCEL;
    }
    
    // Default to cancel for unknown commands (safest option)
    return JobCommand::CANCEL;
}

/**
 * Convert JobCommand to Windows job control constant
 */
#ifdef _WIN32
DWORD jobCommandToWin32(JobCommand cmd) {
    switch (cmd) {
        case JobCommand::PAUSE:
            return JOB_CONTROL_PAUSE;
        case JobCommand::RESUME:
            return JOB_CONTROL_RESUME;
        case JobCommand::CANCEL:
            return JOB_CONTROL_CANCEL;
        default:
            return JOB_CONTROL_CANCEL;
    }
}
#endif

/**
 * Convert JobCommand to CUPS job operation
 */
#ifdef __linux__
ipp_jstate_t jobCommandToCups(JobCommand cmd) {
    switch (cmd) {
        case JobCommand::PAUSE:
            return IPP_JOB_HELD;
        case JobCommand::RESUME:
            return IPP_JOB_PENDING;
        case JobCommand::CANCEL:
            return IPP_JOB_CANCELLED;
        default:
            return IPP_JOB_CANCELLED;
    }
}
#endif

/**
 * Generic job state mapping from string (for backward compatibility)
 */
std::string mapJobStateString(const std::string& statusStr) {
    std::string upperStatus = statusStr;
    std::transform(upperStatus.begin(), upperStatus.end(), upperStatus.begin(), ::toupper);
    
    if (upperStatus.find("COMPLETED") != std::string::npos ||
        upperStatus.find("PRINTED") != std::string::npos) {
        return "completed";
    }
    
    if (upperStatus.find("PRINTING") != std::string::npos ||
        upperStatus.find("PROCESSING") != std::string::npos ||
        upperStatus.find("SPOOLING") != std::string::npos) {
        return "printing";
    }
    
    if (upperStatus.find("CANCELLED") != std::string::npos ||
        upperStatus.find("CANCELED") != std::string::npos ||
        upperStatus.find("DELETED") != std::string::npos) {
        return "canceled";
    }
    
    if (upperStatus.find("ERROR") != std::string::npos ||
        upperStatus.find("ABORTED") != std::string::npos ||
        upperStatus.find("STOPPED") != std::string::npos) {
        return "error";  
    }
    
    // Default to pending for unknown states
    return "pending";
}

} // namespace JobMapping
} // namespace NodePrinter