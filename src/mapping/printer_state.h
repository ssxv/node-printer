// Printer state mapping utilities
// Normalizes OS-specific printer states to standard states

#pragma once
#include <string>
#include <vector>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#include <winspool.h>
#endif

#ifdef __linux__
#include <cups/cups.h>
#endif

namespace NodePrinter {
namespace StateMapping {

/**
 * Map Windows printer status to normalized state
 */
std::string mapPrinterState(uint32_t winStatus, uint32_t winAttributes) {
#ifdef _WIN32
    // Priority order: error > offline > printing > stopped > idle
    
    // Check for error conditions first
    if (winStatus & (PRINTER_STATUS_ERROR | PRINTER_STATUS_NO_TONER | 
                     PRINTER_STATUS_PAPER_JAM | PRINTER_STATUS_PAPER_OUT | 
                     PRINTER_STATUS_PAPER_PROBLEM | PRINTER_STATUS_OUTPUT_BIN_FULL)) {
        return "error";
    }
    
    // Check for offline conditions
    if (winStatus & (PRINTER_STATUS_OFFLINE | PRINTER_STATUS_NOT_AVAILABLE |
                     PRINTER_STATUS_SERVER_UNKNOWN)) {
        return "offline";
    }
    
    // Check for printing/active conditions
    if (winStatus & (PRINTER_STATUS_PRINTING | PRINTER_STATUS_PROCESSING |
                     PRINTER_STATUS_IO_ACTIVE | PRINTER_STATUS_BUSY)) {
        return "printing";
    }
    
    // Check for stopped/paused conditions
    if (winStatus & (PRINTER_STATUS_PAUSED | PRINTER_STATUS_PENDING_DELETION)) {
        return "stopped";
    }
    
    // Check if printer is available but waiting
    if (winStatus & (PRINTER_STATUS_WAITING | PRINTER_STATUS_WARMING_UP |
                     PRINTER_STATUS_INITIALIZING | PRINTER_STATUS_POWER_SAVE)) {
        return "idle";
    }
    
    // Default to idle for normal status or unknown states
    return "idle";
#else
    // Fallback for non-Windows builds
    return "offline";
#endif
}

/**
 * Map CUPS printer state to normalized state
 */
std::string mapCupsPrinterState(int cupsState) {
#ifdef __linux__
    switch (cupsState) {
        case IPP_PRINTER_IDLE:
            return "idle";
        case IPP_PRINTER_PROCESSING:
            return "printing";
        case IPP_PRINTER_STOPPED:
            return "stopped";
        default:
            return "offline";
    }
#else
    // Fallback for non-Linux builds
    return "offline";
#endif
}

/**
 * Generic state mapping from string status arrays (for backward compatibility)
 */
std::string mapStatusArray(const std::vector<std::string>& statusArray) {
    // Check each status in priority order
    for (const auto& status : statusArray) {
        std::string upperStatus = status;
        std::transform(upperStatus.begin(), upperStatus.end(), upperStatus.begin(), ::toupper);
        
        // Error conditions (highest priority)
        if (upperStatus.find("ERROR") != std::string::npos ||
            upperStatus.find("JAM") != std::string::npos ||
            upperStatus.find("NO-TONER") != std::string::npos ||
            upperStatus.find("PAPER-OUT") != std::string::npos) {
            return "error";
        }
        
        // Offline conditions
        if (upperStatus.find("OFFLINE") != std::string::npos ||
            upperStatus.find("NOT-AVAILABLE") != std::string::npos) {
            return "offline";
        }
        
        // Printing conditions
        if (upperStatus.find("PRINTING") != std::string::npos ||
            upperStatus.find("PROCESSING") != std::string::npos ||
            upperStatus.find("BUSY") != std::string::npos) {
            return "printing";
        }
        
        // Stopped conditions
        if (upperStatus.find("PAUSED") != std::string::npos ||
            upperStatus.find("STOPPED") != std::string::npos) {
            return "stopped";
        }
    }
    
    // Default to idle if no specific conditions found
    return "idle";
}

} // namespace StateMapping
} // namespace NodePrinter