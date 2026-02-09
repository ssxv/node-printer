// Windows-specific printer implementation
// Implements IPrinterAPI using Winspool API

#include "../printer_api.h"
#include "../../mapping/printer_state.h"
#include "win_utils.h"
#include <vector>
#include <string>
#include <sstream>

namespace NodePrinter {

using namespace WinUtils;

// Status map from original implementation
static const std::vector<std::pair<std::string, DWORD>>& getStatusMap() {
  static std::vector<std::pair<std::string, DWORD>> v = {
    {"BUSY", PRINTER_STATUS_BUSY},
    {"DOOR-OPEN", PRINTER_STATUS_DOOR_OPEN},
    {"ERROR", PRINTER_STATUS_ERROR},
    {"INITIALIZING", PRINTER_STATUS_INITIALIZING},
    {"IO-ACTIVE", PRINTER_STATUS_IO_ACTIVE},
    {"MANUAL-FEED", PRINTER_STATUS_MANUAL_FEED},
    {"NO-TONER", PRINTER_STATUS_NO_TONER},
    {"NOT-AVAILABLE", PRINTER_STATUS_NOT_AVAILABLE},
    {"OFFLINE", PRINTER_STATUS_OFFLINE},
    {"OUT-OF-MEMORY", PRINTER_STATUS_OUT_OF_MEMORY},
    {"OUTPUT-BIN-FULL", PRINTER_STATUS_OUTPUT_BIN_FULL},
    {"PAGE-PUNT", PRINTER_STATUS_PAGE_PUNT},
    {"PAPER-JAM", PRINTER_STATUS_PAPER_JAM},
    {"PAPER-OUT", PRINTER_STATUS_PAPER_OUT},
    {"PAPER-PROBLEM", PRINTER_STATUS_PAPER_PROBLEM},
    {"PAUSED", PRINTER_STATUS_PAUSED},
    {"PENDING-DELETION", PRINTER_STATUS_PENDING_DELETION},
    {"POWER-SAVE", PRINTER_STATUS_POWER_SAVE},
    {"PRINTING", PRINTER_STATUS_PRINTING},
    {"PROCESSING", PRINTER_STATUS_PROCESSING},
    {"SERVER-UNKNOWN", PRINTER_STATUS_SERVER_UNKNOWN},
    {"TONER-LOW", PRINTER_STATUS_TONER_LOW},
    {"USER-INTERVENTION", PRINTER_STATUS_USER_INTERVENTION},
    {"WAITING", PRINTER_STATUS_WAITING},
    {"WARMING-UP", PRINTER_STATUS_WARMING_UP}
  };
  return v;
}

class WinPrinterAPI : public IPrinterAPI {
public:
  std::vector<PrinterInfo> getPrinters() override {
    std::vector<PrinterInfo> printers;
    
    DWORD needed = 0, returned = 0, flags = PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS;
    
    // Get required buffer size
    EnumPrintersW(flags, NULL, 2, NULL, 0, &needed, &returned);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
      throw std::runtime_error("Failed to enumerate printers");
    }
    
    // Allocate buffer and get printer info
    std::vector<BYTE> buffer(needed);
    PRINTER_INFO_2W* pPrinters = reinterpret_cast<PRINTER_INFO_2W*>(buffer.data());
    
    if (!EnumPrintersW(flags, NULL, 2, buffer.data(), needed, &needed, &returned)) {
      throw std::runtime_error("Failed to get printer information");
    }
    
    // Convert to PrinterInfo structs
    for (DWORD i = 0; i < returned; ++i) {
      PrinterInfo info;
      info.name = WinUtils::ws_to_utf8(pPrinters[i].pPrinterName);
      info.state = StateMapping::mapPrinterState(pPrinters[i].Status, pPrinters[i].Attributes);
      
      if (pPrinters[i].pLocation) {
        info.location = WinUtils::ws_to_utf8(pPrinters[i].pLocation);
      }
      
      if (pPrinters[i].pComment) {
        info.description = WinUtils::ws_to_utf8(pPrinters[i].pComment);
      }
      
      // Check if this is the default printer
      std::string defaultPrinter = getDefaultPrinterName();
      info.isDefault = (info.name == defaultPrinter);
      
      // Basic format support (all Windows printers support RAW)
      info.formats.push_back("RAW");
      info.formats.push_back("TEXT");
      
      printers.push_back(std::move(info));
    }
    
    return printers;
  }
  
  PrinterInfo getPrinter(const std::string& name) override {
    std::wstring wname = WinUtils::utf8_to_ws(name);
    WinUtils::PrinterHandle handle(wname.c_str());
    
    if (!handle.isOk()) {
      throw std::runtime_error("Printer not found: " + name);
    }
    
    DWORD needed = 0;
    GetPrinterW(handle, 2, NULL, 0, &needed);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
      throw std::runtime_error("Failed to get printer info for: " + name);
    }
    
    std::vector<BYTE> buffer(needed);
    PRINTER_INFO_2W* pPrinter = reinterpret_cast<PRINTER_INFO_2W*>(buffer.data());
    
    if (!GetPrinterW(handle, 2, buffer.data(), needed, &needed)) {
      throw std::runtime_error("Failed to get printer details for: " + name);
    }
    
    PrinterInfo info;
    info.name = name;
    info.state = StateMapping::mapPrinterState(pPrinter->Status, pPrinter->Attributes);
    
    if (pPrinter->pLocation) {
      info.location = WinUtils::ws_to_utf8(pPrinter->pLocation);
    }
    
    if (pPrinter->pComment) {
      info.description = WinUtils::ws_to_utf8(pPrinter->pComment);
    }
    
    // Check if this is the default printer
    std::string defaultPrinter = getDefaultPrinterName();
    info.isDefault = (info.name == defaultPrinter);
    
    // Basic format support
    info.formats.push_back("RAW");
    info.formats.push_back("TEXT");
    
    return info;
  }
  
  std::string getDefaultPrinterName() override {
    DWORD size = 0;
    GetDefaultPrinterW(NULL, &size);
    if (size == 0) return "";
    
    std::vector<wchar_t> buffer(size);
    if (!GetDefaultPrinterW(buffer.data(), &size)) return "";
    
    return WinUtils::ws_to_utf8(buffer.data());
  }
  
  std::vector<std::string> getSupportedFormats() override {
    return {"RAW", "TEXT", "COMMAND"};
  }
  
  PrinterCapabilities getCapabilities(const std::string& name) override {
    PrinterCapabilities caps;
    caps.formats = getSupportedFormats();
    
    std::wstring wname = WinUtils::utf8_to_ws(name);
    WinUtils::PrinterHandle handle(wname.c_str());
    
    if (handle.isOk()) {
      // Try to get device capabilities
      int paperCount = DeviceCapabilitiesW(wname.c_str(), NULL, DC_PAPERNAMES, NULL, NULL);
      if (paperCount > 0) {
        std::vector<wchar_t> paperNames(paperCount * 64); // Each name is 64 chars
        if (DeviceCapabilitiesW(wname.c_str(), NULL, DC_PAPERNAMES, paperNames.data(), NULL) > 0) {
          for (int i = 0; i < paperCount; ++i) {
            caps.paperSizes.push_back(WinUtils::ws_to_utf8(&paperNames[i * 64]));
          }
        }
      }
      
      // Check duplex capability
      int duplex = DeviceCapabilitiesW(wname.c_str(), NULL, DC_DUPLEX, NULL, NULL);
      caps.duplex = (duplex == 1);
      
      // For color capability, we'd need more complex checks
      caps.color = false; // Conservative default
    }
    
    return caps;
  }
  
  Napi::Object getDriverOptions(const std::string& name, Napi::Env env) override {
    std::wstring wname = WinUtils::utf8_to_ws(name);
    WinUtils::PrinterHandle handle(wname.c_str());
    
    Napi::Object options = Napi::Object::New(env);
    
    if (!handle.isOk()) {
      return options;
    }
    
    DWORD needed = 0;
    GetPrinterW(handle, 2, NULL, 0, &needed);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
      return options;
    }
    
    std::vector<BYTE> buffer(needed);
    PRINTER_INFO_2W* pPrinter = reinterpret_cast<PRINTER_INFO_2W*>(buffer.data());
    
    if (!GetPrinterW(handle, 2, buffer.data(), needed, &needed)) {
      return options;
    }
    
    // Add available Windows-specific options
    options.Set("Status", pPrinter->Status);
    options.Set("Attributes", pPrinter->Attributes);
    options.Set("Priority", pPrinter->Priority);
    options.Set("DefaultPriority", pPrinter->DefaultPriority);
    
    if (pPrinter->pDriverName) {
      options.Set("DriverName", WinUtils::ws_to_utf8(pPrinter->pDriverName));
    }
    
    if (pPrinter->pPortName) {
      options.Set("PortName", WinUtils::ws_to_utf8(pPrinter->pPortName));
    }
    
    if (pPrinter->pPrintProcessor) {
      options.Set("PrintProcessor", WinUtils::ws_to_utf8(pPrinter->pPrintProcessor));
    }
    
    if (pPrinter->pDatatype) {
      options.Set("Datatype", WinUtils::ws_to_utf8(pPrinter->pDatatype));
    }
    
    return options;
  }
};

} // namespace NodePrinter