// CUPS-specific printer implementation
// Implements IPrinterAPI using CUPS API

#include "../printer_api.h"
#include "../../mapping/printer_state.h"
#include <cups/cups.h>
#include <cups/ppd.h>
#include <vector>
#include <string>
#include <map>
#include <mutex>

namespace NodePrinter {

// CUPS is not thread-safe, so we need a global mutex
static std::mutex g_cupsMutex;

class CupsPrinterAPI : public IPrinterAPI {
public:
  std::vector<PrinterInfo> getPrinters() override {
    std::lock_guard<std::mutex> lock(g_cupsMutex);
    std::vector<PrinterInfo> printers;
    
    cups_dest_t* dests = nullptr;
    int num_dests = cupsGetDests(&dests);
    
    if (num_dests < 0) {
      throw std::runtime_error("Failed to get printers from CUPS");
    }
    
    for (int i = 0; i < num_dests; ++i) {
      PrinterInfo info;
      info.name = dests[i].name;
      info.isDefault = dests[i].is_default;
      
      // Get printer state
      ipp_pstate_t state = static_cast<ipp_pstate_t>(
        cupsGetIntegerOption("printer-state", dests[i].num_options, dests[i].options));
      info.state = StateMapping::mapCupsPrinterState(state);
      
      // Get location and description from options
      const char* location = cupsGetOption("printer-location", dests[i].num_options, dests[i].options);
      if (location) {
        info.location = location;
      }
      
      const char* description = cupsGetOption("printer-info", dests[i].num_options, dests[i].options);
      if (description) {
        info.description = description;
      }
      
      // Basic format support
      info.formats.push_back("RAW");
      info.formats.push_back("TEXT");
      
      // Check for additional formats
      const char* acceptedTypes = cupsGetOption("document-format-supported", 
                                                dests[i].num_options, dests[i].options);
      if (acceptedTypes) {
        std::string types(acceptedTypes);
        if (types.find("application/pdf") != std::string::npos) {
          info.formats.push_back("PDF");
        }
        if (types.find("image/") != std::string::npos) {
          info.formats.push_back("IMAGE");
        }
      }
      
      printers.push_back(std::move(info));
    }
    
    cupsFreeDests(num_dests, dests);
    return printers;
  }
  
  PrinterInfo getPrinter(const std::string& name) override {
    std::lock_guard<std::mutex> lock(g_cupsMutex);
    
    cups_dest_t* dest = cupsGetNamedDest(CUPS_HTTP_DEFAULT, name.c_str(), NULL);
    if (!dest) {
      throw std::runtime_error("Printer not found: " + name);
    }
    
    PrinterInfo info;
    info.name = name;
    info.isDefault = dest->is_default;
    
    // Get printer state
    ipp_pstate_t state = static_cast<ipp_pstate_t>(
      cupsGetIntegerOption("printer-state", dest->num_options, dest->options));
    info.state = StateMapping::mapCupsPrinterState(state);
    
    // Get location and description
    const char* location = cupsGetOption("printer-location", dest->num_options, dest->options);
    if (location) {
      info.location = location;
    }
    
    const char* description = cupsGetOption("printer-info", dest->num_options, dest->options);
    if (description) {
      info.description = description;
    }
    
    // Get supported formats
    info.formats.push_back("RAW");
    info.formats.push_back("TEXT");
    
    const char* acceptedTypes = cupsGetOption("document-format-supported", 
                                              dest->num_options, dest->options);
    if (acceptedTypes) {
      std::string types(acceptedTypes);
      if (types.find("application/pdf") != std::string::npos) {
        info.formats.push_back("PDF");
      }
      if (types.find("image/") != std::string::npos) {
        info.formats.push_back("IMAGE");
      }
    }
    
    cupsFreeDests(1, dest);
    return info;
  }
  
  std::string getDefaultPrinterName() override {
    std::lock_guard<std::mutex> lock(g_cupsMutex);
    
    const char* defaultPrinter = cupsGetDefault();
    return defaultPrinter ? std::string(defaultPrinter) : std::string();
  }
  
  std::vector<std::string> getSupportedFormats() override {
    return {
      "RAW",
      "TEXT",
      "PDF", 
      "POSTSCRIPT",
      "IMAGE",
      "AUTO"
    };
  }
  
  PrinterCapabilities getCapabilities(const std::string& name) override {
    std::lock_guard<std::mutex> lock(g_cupsMutex);
    
    PrinterCapabilities caps;
    caps.formats = {"RAW", "TEXT"};
    
    cups_dest_t* dest = cupsGetNamedDest(CUPS_HTTP_DEFAULT, name.c_str(), NULL);
    if (!dest) {
      return caps;
    }
    
    // Get supported formats
    const char* acceptedTypes = cupsGetOption("document-format-supported", 
                                              dest->num_options, dest->options);
    if (acceptedTypes) {
      std::string types(acceptedTypes);
      if (types.find("application/pdf") != std::string::npos) {
        caps.formats.push_back("PDF");
      }
      if (types.find("image/") != std::string::npos) {
        caps.formats.push_back("IMAGE");
      }
    }
    
    // Use basic capabilities without deprecated PPD APIs
    // Modern CUPS provides capabilities through destination info
    // For now, provide basic capabilities to avoid deprecated APIs
    
    // Check for standard options that indicate capabilities
    const char* colorSupport = cupsGetOption("print-color-mode-supported", dest->num_options, dest->options);
    if (colorSupport && strstr(colorSupport, "color")) {
      caps.color = true;
    }
    
    const char* duplexSupport = cupsGetOption("sides-supported", dest->num_options, dest->options);
    if (duplexSupport && strstr(duplexSupport, "two-sided")) {
      caps.duplex = true;
    }
    
    // Add common paper sizes without PPD
    caps.paperSizes = {"Letter", "A4", "Legal", "A3", "A5", "Executive", "Tabloid"};
    
    cupsFreeDests(1, dest);
    return caps;
  }
  
  Napi::Object getDriverOptions(const std::string& name, Napi::Env env) override {
    std::lock_guard<std::mutex> lock(g_cupsMutex);
    
    Napi::Object options = Napi::Object::New(env);
    
    cups_dest_t* dest = cupsGetNamedDest(CUPS_HTTP_DEFAULT, name.c_str(), NULL);
    if (!dest) {
      return options;
    }
    
    // Add all CUPS options as key-value pairs
    for (int i = 0; i < dest->num_options; ++i) {
      options.Set(dest->options[i].name, dest->options[i].value);
    }
    
    // Enhanced driver options using modern CUPS APIs (avoiding deprecated PPD)
    // For now, return basic options from CUPS destination to avoid deprecated APIs
    // Future enhancement could use cupsCopyDestInfo for more detailed options
    
    cupsFreeDests(1, dest);
    return options;
  }
};

} // namespace NodePrinter