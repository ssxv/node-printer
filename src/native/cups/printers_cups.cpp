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
    
    // Try to get PPD for more detailed capabilities
    const char* ppdFile = cupsGetPPD(name.c_str());
    if (ppdFile) {
      ppd_file_t* ppd = ppdOpenFile(ppdFile);
      if (ppd) {
        // Get paper sizes
        ppd_size_t* sizes = ppd->sizes;
        for (int i = 0; i < ppd->num_sizes; ++i) {
          caps.paperSizes.push_back(sizes[i].name);
        }
        
        // Check for duplex capability
        ppd_option_t* duplexOption = ppdFindOption(ppd, "Duplex");
        caps.duplex = (duplexOption != nullptr);
        
        // Check for color capability  
        ppd_option_t* colorOption = ppdFindOption(ppd, "ColorModel");
        caps.color = (colorOption != nullptr);
        
        ppdClose(ppd);
      }
      unlink(ppdFile); // Remove temporary PPD file
    }
    
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
    
    // Try to get additional PPD options
    const char* ppdFile = cupsGetPPD(name.c_str());
    if (ppdFile) {
      ppd_file_t* ppd = ppdOpenFile(ppdFile);
      if (ppd) {
        Napi::Object ppdOptions = Napi::Object::New(env);
        
        // Add groups and options from PPD
        for (int i = 0; i < ppd->num_groups; ++i) {
          ppd_group_t* group = &ppd->groups[i];
          Napi::Object groupObj = Napi::Object::New(env);
          
          for (int j = 0; j < group->num_options; ++j) {
            ppd_option_t* option = &group->options[j];
            Napi::Array choices = Napi::Array::New(env);
            
            for (int k = 0; k < option->num_choices; ++k) {
              Napi::Object choice = Napi::Object::New(env);
              choice.Set("choice", option->choices[k].choice);
              choice.Set("text", option->choices[k].text);
              choices.Set(k, choice);
            }
            
            Napi::Object optionObj = Napi::Object::New(env);
            optionObj.Set("keyword", option->keyword);
            optionObj.Set("text", option->text);
            optionObj.Set("choices", choices);
            
            groupObj.Set(option->keyword, optionObj);
          }
          
          ppdOptions.Set(group->name, groupObj);
        }
        options.Set("ppd", ppdOptions);
        
        ppdClose(ppd);
      }
      unlink(ppdFile);
    }
    
    cupsFreeDests(1, dest);
    return options;
  }
};

} // namespace NodePrinter