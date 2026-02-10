#pragma once
#include <napi.h>
#include <vector>
#include <string>
#include <memory>

namespace NodePrinter {

/**
 * Cross-platform printer information
 * All OS-specific data is normalized to this structure
 */
struct PrinterInfo {
    std::string name;
    bool isDefault = false;
    std::string state;        // normalized: "idle", "printing", "stopped", "offline", "error"  
    std::string location;
    std::string description;
    
    // Optional extended information
    std::vector<std::string> formats;
    std::vector<std::string> paperSizes;
    bool supportsDuplex = false;
    bool supportsColor = false;
};

/**
 * Cross-platform printer capabilities
 */
struct PrinterCapabilities {
    std::vector<std::string> formats;
    std::vector<std::string> paperSizes;
    bool duplex;
    bool color;
};

/**
 * Abstract printer API interface
 * Platform-specific implementations must inherit from this
 */
class IPrinterAPI {
public:
    virtual ~IPrinterAPI() = default;
    
    /**
     * Get list of all available printers
     */
    virtual std::vector<PrinterInfo> getPrinters() = 0;
    
    /**
     * Get information about a specific printer
     * @param name Printer name
     * @throws std::runtime_error if printer not found
     */
    virtual PrinterInfo getPrinter(const std::string& name) = 0;
    
    /**
     * Get the default printer name
     * @returns Empty string if no default printer
     */
    virtual std::string getDefaultPrinterName() = 0;
    
    /**
     * Get supported print formats for the system
     */
    virtual std::vector<std::string> getSupportedFormats() = 0;
    
    /**
     * Get printer capabilities
     * @param name Printer name
     */
    virtual PrinterCapabilities getCapabilities(const std::string& name) = 0;
    
    /**
     * Get raw driver options (platform-specific data)
     * @param name Printer name
     * @returns Platform-specific options as key-value pairs
     */
    virtual Napi::Object getDriverOptions(const std::string& name, Napi::Env env) = 0;
};

/**
 * Factory function to create platform-specific printer API
 */
std::unique_ptr<IPrinterAPI> createPrinterAPI();

/**
 * Utility functions for state mapping
 */
namespace StateMapping {
    std::string mapPrinterState(uint32_t winStatus, uint32_t winAttributes);
    std::string mapCupsPrinterState(int cupsState);
}

} // namespace NodePrinter