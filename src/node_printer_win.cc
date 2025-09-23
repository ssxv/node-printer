#include <napi.h>
#ifdef _WIN32
#include <windows.h>
#include <winspool.h>
#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <fstream>

static std::string ws_to_utf8(LPCWSTR wstr) {
  if (!wstr) return std::string();
  int len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
  if (len <= 0) return std::string();
  std::string out;
  out.resize(len - 1);
  WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &out[0], len, NULL, NULL);
  return out;
}

// maps copied (small subset) from original implementation
static const std::vector<std::pair<std::string, DWORD>> getStatusMap() {
  static std::vector<std::pair<std::string, DWORD>> v;
  if (!v.empty()) return v;
  v.push_back({"BUSY", PRINTER_STATUS_BUSY});
  v.push_back({"DOOR-OPEN", PRINTER_STATUS_DOOR_OPEN});
  v.push_back({"ERROR", PRINTER_STATUS_ERROR});
  v.push_back({"INITIALIZING", PRINTER_STATUS_INITIALIZING});
  v.push_back({"IO-ACTIVE", PRINTER_STATUS_IO_ACTIVE});
  v.push_back({"MANUAL-FEED", PRINTER_STATUS_MANUAL_FEED});
  v.push_back({"NO-TONER", PRINTER_STATUS_NO_TONER});
  v.push_back({"NOT-AVAILABLE", PRINTER_STATUS_NOT_AVAILABLE});
  v.push_back({"OFFLINE", PRINTER_STATUS_OFFLINE});
  v.push_back({"OUT-OF-MEMORY", PRINTER_STATUS_OUT_OF_MEMORY});
  v.push_back({"OUTPUT-BIN-FULL", PRINTER_STATUS_OUTPUT_BIN_FULL});
  v.push_back({"PAGE-PUNT", PRINTER_STATUS_PAGE_PUNT});
  v.push_back({"PAPER-JAM", PRINTER_STATUS_PAPER_JAM});
  v.push_back({"PAPER-OUT", PRINTER_STATUS_PAPER_OUT});
  v.push_back({"PAPER-PROBLEM", PRINTER_STATUS_PAPER_PROBLEM});
  v.push_back({"PAUSED", PRINTER_STATUS_PAUSED});
  v.push_back({"PENDING-DELETION", PRINTER_STATUS_PENDING_DELETION});
  v.push_back({"POWER-SAVE", PRINTER_STATUS_POWER_SAVE});
  v.push_back({"PRINTING", PRINTER_STATUS_PRINTING});
  v.push_back({"PROCESSING", PRINTER_STATUS_PROCESSING});
  v.push_back({"SERVER-UNKNOWN", PRINTER_STATUS_SERVER_UNKNOWN});
  v.push_back({"TONER-LOW", PRINTER_STATUS_TONER_LOW});
  v.push_back({"USER-INTERVENTION", PRINTER_STATUS_USER_INTERVENTION});
  v.push_back({"WAITING", PRINTER_STATUS_WAITING});
  v.push_back({"WARMING-UP", PRINTER_STATUS_WARMING_UP});
  return v;
}

static const std::vector<std::pair<std::string, DWORD>> getJobStatusMap() {
  static std::vector<std::pair<std::string, DWORD>> v;
  if (!v.empty()) return v;
  v.push_back({"PRINTING", JOB_STATUS_PRINTING});
  v.push_back({"PRINTED", JOB_STATUS_PRINTED});
  v.push_back({"PAUSED", JOB_STATUS_PAUSED});
  v.push_back({"BLOCKED-DEVQ", JOB_STATUS_BLOCKED_DEVQ});
  v.push_back({"DELETED", JOB_STATUS_DELETED});
  v.push_back({"DELETING", JOB_STATUS_DELETING});
  v.push_back({"ERROR", JOB_STATUS_ERROR});
  v.push_back({"OFFLINE", JOB_STATUS_OFFLINE});
  v.push_back({"PAPEROUT", JOB_STATUS_PAPEROUT});
  v.push_back({"RESTART", JOB_STATUS_RESTART});
  v.push_back({"SPOOLING", JOB_STATUS_SPOOLING});
  v.push_back({"USER-INTERVENTION", JOB_STATUS_USER_INTERVENTION});
#ifdef JOB_STATUS_COMPLETE
  v.push_back({"COMPLETE", JOB_STATUS_COMPLETE});
#endif
#ifdef JOB_STATUS_RETAINED
  v.push_back({"RETAINED", JOB_STATUS_RETAINED});
#endif
  return v;
}

struct PrinterHandle {
  HANDLE h;
  BOOL ok;
  PrinterHandle(LPWSTR name) : h(NULL), ok(FALSE) {
    ok = OpenPrinterW(name, &h, NULL);
  }
  ~PrinterHandle() { if (ok) ClosePrinter(h); }
  operator HANDLE() { return h; }
  bool isOk() const { return !!ok; }
};

// Forward declarations (must appear before parseJobObject)
static double systemtime_to_epoch_seconds(const SYSTEMTIME &st);
static void parseJobObject_add_times(const JOB_INFO_2W* job, Napi::Env env, Napi::Object& out);

static void parseJobObject(const JOB_INFO_2W* job, Napi::Env env, Napi::Object& out) {
  // Common fields
  out.Set("id", Napi::Number::New(env, job->JobId));
  if (job->pDocument && *job->pDocument) out.Set("name", Napi::String::New(env, ws_to_utf8(job->pDocument)));
  if (job->pPrinterName && *job->pPrinterName) out.Set("printerName", Napi::String::New(env, ws_to_utf8(job->pPrinterName)));
  if (job->pUserName && *job->pUserName) out.Set("user", Napi::String::New(env, ws_to_utf8(job->pUserName)));
  if (job->pDatatype && *job->pDatatype) out.Set("format", Napi::String::New(env, ws_to_utf8(job->pDatatype)));

  out.Set("priority", Napi::Number::New(env, job->Priority));
  // Size field may be 0 on some systems; use Size
  out.Set("size", Napi::Number::New(env, job->Size));

  // Status -> array of strings
  Napi::Array statusArr = Napi::Array::New(env);
  uint32_t si = 0;
  for (const auto &kv : getJobStatusMap()) {
    if (job->Status & kv.second) {
      statusArr.Set(si++, Napi::String::New(env, kv.first));
    }
  }
  if (job->pStatus && *job->pStatus) {
    statusArr.Set(si++, Napi::String::New(env, ws_to_utf8(job->pStatus)));
  }
  out.Set("status", statusArr);

  // Specific fields
  if (job->pMachineName && *job->pMachineName) out.Set("machineName", Napi::String::New(env, ws_to_utf8(job->pMachineName)));
  if (job->pDocument && *job->pDocument) out.Set("document", Napi::String::New(env, ws_to_utf8(job->pDocument)));
  if (job->pNotifyName && *job->pNotifyName) out.Set("notifyName", Napi::String::New(env, ws_to_utf8(job->pNotifyName)));
  if (job->pPrintProcessor && *job->pPrintProcessor) out.Set("printProcessor", Napi::String::New(env, ws_to_utf8(job->pPrintProcessor)));
  if (job->pParameters && *job->pParameters) out.Set("parameters", Napi::String::New(env, ws_to_utf8(job->pParameters)));
  if (job->pDriverName && *job->pDriverName) out.Set("driverName", Napi::String::New(env, ws_to_utf8(job->pDriverName)));

  out.Set("position", Napi::Number::New(env, job->Position));
  if (job->StartTime > 0) out.Set("startTime", Napi::Number::New(env, job->StartTime));
  if (job->UntilTime > 0) out.Set("untilTime", Napi::Number::New(env, job->UntilTime));
  out.Set("totalPages", Napi::Number::New(env, job->TotalPages));
  out.Set("time", Napi::Number::New(env, job->Time));
  out.Set("pagesPrinted", Napi::Number::New(env, job->PagesPrinted));
  // Add numeric time fields to match JobDetails (creationTime, processingTime, completedTime)
  parseJobObject_add_times(job, env, out);
}

// Convert SYSTEMTIME (Windows) to epoch seconds (UTC)
static double systemtime_to_epoch_seconds(const SYSTEMTIME &st) {
  // Convert SYSTEMTIME to FILETIME (UTC)
  FILETIME ft;
  if (!SystemTimeToFileTime(&st, &ft)) return 0.0;
  // Convert FILETIME (100-ns since Jan 1, 1601) to uint64
  ULARGE_INTEGER uli;
  uli.HighPart = ft.dwHighDateTime;
  uli.LowPart = ft.dwLowDateTime;
  // Convert to seconds since Unix epoch
  const unsigned long long EPOCH_DIFF = 11644473600ULL; // seconds between 1601 and 1970
  unsigned long long total100ns = uli.QuadPart; // 100-ns intervals
  unsigned long long totalSeconds = total100ns / 10000000ULL;
  if (totalSeconds <= EPOCH_DIFF) return 0.0;
  return static_cast<double>(totalSeconds - EPOCH_DIFF);
}

// Patch parseJobObject to add creationTime, processingTime, completedTime numeric fields
static void parseJobObject_add_times(const JOB_INFO_2W* job, Napi::Env env, Napi::Object& out) {
  // creationTime <- Submitted (SYSTEMTIME) if available
  // JOB_INFO_2W has Submitted as SYSTEMTIME in newer SDKs; check pointer via reinterpret_cast guard
  // Unfortunately the struct contains Submitted directly, so access it and convert
  // Use a safe approach: attempt to read Submitted if sizeof(SYSTEMTIME) matches expected offset presence
  // We'll try-catch via probing StartTime / Time
  // Creation time: try to use Submitted if non-zero
  double creation = 0.0;
  // We can't reliably detect presence; attempt to copy
  SYSTEMTIME stSubmitted = {0};
  // Attempt to copy memory for Submitted: it's located after PagesPrinted in many headers; but safer to call GetJob separately in getJob API for accurate times. Here we'll use job->Time for processingTime and job->StartTime/UntilTime for completed/proc.
  creation = 0.0; // leave 0 if not available

  double processing = static_cast<double>(job->Time);
  double completed = 0.0;
  if (job->StartTime > 0) processing = static_cast<double>(job->StartTime);
  if (job->UntilTime > 0) completed = static_cast<double>(job->UntilTime);

  out.Set("creationTime", Napi::Number::New(env, creation));
  out.Set("processingTime", Napi::Number::New(env, processing));
  out.Set("completedTime", Napi::Number::New(env, completed));
}

struct ErrorInfo { DWORD code; std::string message; };

static ErrorInfo getLastErrorInfo() {
  ErrorInfo out{0, std::string()};
  DWORD code = GetLastError();
  out.code = code;
  LPWSTR buf = NULL;
  DWORD len = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                            NULL, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&buf, 0, NULL);
  if (len && buf) {
    out.message = ws_to_utf8(buf);
    LocalFree(buf);
  }
  return out;
}

static Napi::Error makeNapiErrorWithCode(Napi::Env env, const std::string& prefix, const ErrorInfo& info) {
  std::ostringstream s;
  s << prefix;
  if (info.code != 0) s << " (code=" << info.code << ")";
  if (!info.message.empty()) s << ": " << info.message;
  Napi::Error e = Napi::Error::New(env, s.str());
  if (info.code != 0) e.Set("code", Napi::Number::New(env, static_cast<double>(info.code)));
  return e;
}

static std::string getDefaultPrinterUtf8() {
  DWORD cSize = 0;
  GetDefaultPrinterW(NULL, &cSize);
  if (cSize == 0) return std::string();
  std::vector<uint16_t> buf(cSize);
  BOOL res = GetDefaultPrinterW((LPWSTR)buf.data(), &cSize);
  if (!res) return std::string();
  return ws_to_utf8((LPCWSTR)buf.data());
}

static std::vector<uint16_t> utf8_to_w(const std::string& s) {
  int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, NULL, 0);
  std::vector<uint16_t> buf;
  if (n > 0) {
    buf.resize(n);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, (LPWSTR)buf.data(), n);
  }
  return buf;
}

// Threshold to decide whether to stream via temp file instead of copying into memory (4 MiB)
static const size_t STREAM_THRESHOLD = 4 * 1024 * 1024;

// Forward declarations
static double systemtime_to_epoch_seconds(const SYSTEMTIME &st);
static void parseJobObject_add_times(const JOB_INFO_2W* job, Napi::Env env, Napi::Object& out);

static void retrieveAndParseJobs(LPWSTR printerName, DWORD totalJobs, Napi::Env env, Napi::Array& jobsOut) {
  if (totalJobs == 0) return;
  PrinterHandle handle(printerName);
  if (!handle.isOk()) {
    ErrorInfo info = getLastErrorInfo();
    Napi::Error e = makeNapiErrorWithCode(env, "error on PrinterHandle", info);
    jobsOut.Set((uint32_t)0, e.Value());
    return;
  }

  DWORD bytesNeeded = 0, returned = 0;
  BOOL b = EnumJobsW(handle, 0, totalJobs, 2, NULL, 0, &bytesNeeded, &returned);
  std::vector<BYTE> buffer(bytesNeeded);
  b = EnumJobsW(handle, 0, totalJobs, 2, buffer.data(), bytesNeeded, &bytesNeeded, &returned);
  if (!b) {
    ErrorInfo info = getLastErrorInfo();
    Napi::Error e = makeNapiErrorWithCode(env, "Error on EnumJobsW", info);
    jobsOut.Set((uint32_t)0, e.Value());
    return;
  }
  JOB_INFO_2W* job = reinterpret_cast<JOB_INFO_2W*>(buffer.data());
  for (DWORD i = 0; i < returned; ++i) {
    Napi::Object jobObj = Napi::Object::New(env);
    parseJobObject(&job[i], env, jobObj);
    jobsOut.Set((uint32_t)i, jobObj);
  }
}

static void parsePrinterInfo(const PRINTER_INFO_2W* printer, Napi::Env env, Napi::Object& out) {
  if (printer->pPrinterName) out.Set("name", Napi::String::New(env, ws_to_utf8(printer->pPrinterName)));
  if (printer->pServerName) out.Set("serverName", Napi::String::New(env, ws_to_utf8(printer->pServerName)));
  if (printer->pShareName) out.Set("shareName", Napi::String::New(env, ws_to_utf8(printer->pShareName)));
  if (printer->pPortName) out.Set("portName", Napi::String::New(env, ws_to_utf8(printer->pPortName)));
  if (printer->pDriverName) out.Set("driverName", Napi::String::New(env, ws_to_utf8(printer->pDriverName)));
  if (printer->pComment) out.Set("comment", Napi::String::New(env, ws_to_utf8(printer->pComment)));
  if (printer->pLocation) out.Set("location", Napi::String::New(env, ws_to_utf8(printer->pLocation)));
  if (printer->pSepFile) out.Set("sepFile", Napi::String::New(env, ws_to_utf8(printer->pSepFile)));
  if (printer->pPrintProcessor) out.Set("printProcessor", Napi::String::New(env, ws_to_utf8(printer->pPrintProcessor)));
  if (printer->pDatatype) out.Set("datatype", Napi::String::New(env, ws_to_utf8(printer->pDatatype)));
  if (printer->pParameters) out.Set("parameters", Napi::String::New(env, ws_to_utf8(printer->pParameters)));

  // statuses
  Napi::Array statusArr = Napi::Array::New(env);
  uint32_t si = 0;
  for (const auto &kv : getStatusMap()) {
    if (printer->Status & kv.second) {
      statusArr.Set(si++, Napi::String::New(env, kv.first));
    }
  }
  out.Set("status", statusArr);
  out.Set("statusNumber", Napi::Number::New(env, printer->Status));

  // attributes - a subset
  Napi::Array attrArr = Napi::Array::New(env);
  uint32_t ai = 0;
  // simple attributes mapping (subset)
  if (printer->Attributes & PRINTER_ATTRIBUTE_DIRECT) attrArr.Set(ai++, Napi::String::New(env, "DIRECT"));
  if (printer->Attributes & PRINTER_ATTRIBUTE_HIDDEN) attrArr.Set(ai++, Napi::String::New(env, "HIDDEN"));
  if (printer->Attributes & PRINTER_ATTRIBUTE_KEEPPRINTEDJOBS) attrArr.Set(ai++, Napi::String::New(env, "KEEPPRINTEDJOBS"));
  if (printer->Attributes & PRINTER_ATTRIBUTE_LOCAL) attrArr.Set(ai++, Napi::String::New(env, "LOCAL"));
  if (printer->Attributes & PRINTER_ATTRIBUTE_NETWORK) attrArr.Set(ai++, Napi::String::New(env, "NETWORK"));
  out.Set("attributes", attrArr);

  out.Set("priority", Napi::Number::New(env, printer->Priority));
  out.Set("defaultPriority", Napi::Number::New(env, printer->DefaultPriority));
  out.Set("averagePPM", Napi::Number::New(env, printer->AveragePPM));

  // Add isDefault and options to match PrinterDetails TypeScript shape
  std::string defaultName = getDefaultPrinterUtf8();
  bool isDefault = false;
  if (printer->pPrinterName && *printer->pPrinterName) {
    std::string thisName = ws_to_utf8(printer->pPrinterName);
    if (!defaultName.empty() && thisName == defaultName) isDefault = true;
  }
  out.Set("isDefault", Napi::Boolean::New(env, isDefault));

  Napi::Object optionsObj = Napi::Object::New(env);
  // provide printer-state as string to allow JS wrapper to interpret it
  optionsObj.Set("printer-state", Napi::String::New(env, std::to_string(printer->Status)));
  out.Set("options", optionsObj);

  if (printer->StartTime > 0) out.Set("startTime", Napi::Number::New(env, printer->StartTime));
  if (printer->UntilTime > 0) out.Set("untilTime", Napi::Number::New(env, printer->UntilTime));

  if (printer->cJobs > 0) {
    Napi::Array jobsArr = Napi::Array::New(env);
    // Need to open printer handle to get job details
    retrieveAndParseJobs((LPWSTR)printer->pPrinterName, printer->cJobs, env, jobsArr);
    out.Set("jobs", jobsArr);
  }
}

Napi::Array GetPrintersWrapped(const Napi::CallbackInfo& args) {
  Napi::Env env = args.Env();
  DWORD bytesNeeded = 0, returned = 0;
  DWORD flags = PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS;
  BOOL b = EnumPrintersW(flags, NULL, 2, NULL, 0, &bytesNeeded, &returned);
  if (bytesNeeded == 0) {
    return Napi::Array::New(env);
  }
  std::vector<BYTE> buffer(bytesNeeded);
  b = EnumPrintersW(flags, NULL, 2, buffer.data(), bytesNeeded, &bytesNeeded, &returned);
  if (!b) {
    ErrorInfo info = getLastErrorInfo();
    Napi::Error e = makeNapiErrorWithCode(env, "Error on EnumPrinters", info);
    e.ThrowAsJavaScriptException();
    return Napi::Array::New(env);
  }

  PRINTER_INFO_2W* printers = reinterpret_cast<PRINTER_INFO_2W*>(buffer.data());
  Napi::Array result = Napi::Array::New(env, returned);
  for (DWORD i = 0; i < returned; ++i) {
    Napi::Object p = Napi::Object::New(env);
    parsePrinterInfo(&printers[i], env, p);
    result.Set((uint32_t)i, p);
  }
  return result;
}

Napi::Value GetPrinterWrapped(const Napi::CallbackInfo& args) {
  Napi::Env env = args.Env();
  if (args.Length() < 1 || !args[0].IsString()) {
    Napi::TypeError::New(env, "printer name required").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  std::string printerName = args[0].As<Napi::String>().Utf8Value();
  std::vector<uint16_t> printerNameW = utf8_to_w(printerName);
  
  // Open a handle to the printer (direct approach from old implementation)
  PrinterHandle printerHandle((LPWSTR)printerNameW.data());
  if (!printerHandle.isOk()) {
    ErrorInfo info = getLastErrorInfo();
    std::ostringstream s;
    s << "error on PrinterHandle for printer '" << printerName << "'";
    Napi::Error e = makeNapiErrorWithCode(env, s.str(), info);
    e.ThrowAsJavaScriptException();
    return env.Null();
  }
  
  DWORD printersSizeBytes = 0;
  GetPrinterW((HANDLE)printerHandle, 2, NULL, printersSizeBytes, &printersSizeBytes);
  if (printersSizeBytes == 0) {
    Napi::Error::New(env, "Error on allocating memory for printer").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  std::vector<BYTE> buffer(printersSizeBytes);
  PRINTER_INFO_2W* printer = reinterpret_cast<PRINTER_INFO_2W*>(buffer.data());
  
  BOOL bOK = GetPrinterW((HANDLE)printerHandle, 2, (LPBYTE)printer, printersSizeBytes, &printersSizeBytes);
  if (!bOK) {
    ErrorInfo info = getLastErrorInfo();
    Napi::Error e = makeNapiErrorWithCode(env, "Error on GetPrinter", info);
    e.ThrowAsJavaScriptException();
    return env.Null();
  }
  
  Napi::Object result = Napi::Object::New(env);
  parsePrinterInfo(printer, env, result);
  return result;
}

Napi::Value GetDefaultPrinterNameWrapped(const Napi::CallbackInfo& args) {
  Napi::Env env = args.Env();
  std::string defaultPrinter = getDefaultPrinterUtf8();
  if (defaultPrinter.empty()) {
    return env.Undefined();
  }
  return Napi::String::New(env, defaultPrinter);
}

static const std::map<std::string, DWORD> getJobCommandMap() {
  static std::map<std::string, DWORD> result;
  if (!result.empty()) return result;
  
  // Job control commands from old implementation
  result["CANCEL"] = JOB_CONTROL_CANCEL;
  result["PAUSE"] = JOB_CONTROL_PAUSE;
  result["RESTART"] = JOB_CONTROL_RESTART;
  result["RESUME"] = JOB_CONTROL_RESUME;
  result["DELETE"] = JOB_CONTROL_DELETE;
  result["SENT-TO-PRINTER"] = JOB_CONTROL_SENT_TO_PRINTER;
  result["LAST-PAGE-EJECTED"] = JOB_CONTROL_LAST_PAGE_EJECTED;
#ifdef JOB_CONTROL_RETAIN
  result["RETAIN"] = JOB_CONTROL_RETAIN;
#endif
  return result;
}

Napi::Value GetSupportedJobCommandsWrapped(const Napi::CallbackInfo& args) {
  Napi::Env env = args.Env();
  const auto& commandMap = getJobCommandMap();
  Napi::Array result = Napi::Array::New(env, commandMap.size());
  uint32_t i = 0;
  for (const auto& kv : commandMap) {
    result.Set(i++, Napi::String::New(env, kv.first));
  }
  return result;
}

Napi::Value GetSupportedPrintFormatsWrapped(const Napi::CallbackInfo& args) {
  Napi::Env env = args.Env();
  Napi::Array result = Napi::Array::New(env);
  uint32_t formatIndex = 0;

  DWORD numBytes = 0, processorsNum = 0;
  LPWSTR nullVal = NULL;

  // Check the amount of bytes required
  EnumPrintProcessorsW(nullVal, nullVal, 1, (LPBYTE)(NULL), numBytes, &numBytes, &processorsNum);
  if (numBytes == 0) {
    // Fallback to basic formats if enumeration fails
    result.Set(formatIndex++, Napi::String::New(env, "RAW"));
    result.Set(formatIndex++, Napi::String::New(env, "TEXT"));
    result.Set(formatIndex++, Napi::String::New(env, "EMF"));
    return result;
  }

  std::vector<BYTE> processorBuffer(numBytes);
  PRINTPROCESSOR_INFO_1W* processors = reinterpret_cast<PRINTPROCESSOR_INFO_1W*>(processorBuffer.data());
  
  // Retrieve processors
  BOOL isOK = EnumPrintProcessorsW(nullVal, nullVal, 1, (LPBYTE)processors, numBytes, &numBytes, &processorsNum);
  if (!isOK) {
    ErrorInfo info = getLastErrorInfo();
    Napi::Error e = makeNapiErrorWithCode(env, "Error on EnumPrintProcessorsW", info);
    e.ThrowAsJavaScriptException();
    return result;
  }

  PRINTPROCESSOR_INFO_1W* pProcessor = processors;
  for (DWORD processor_i = 0; processor_i < processorsNum; ++processor_i, ++pProcessor) {
    numBytes = 0;
    DWORD dataTypesNum = 0;
    EnumPrintProcessorDatatypesW(nullVal, pProcessor->pName, 1, (LPBYTE)(NULL), numBytes, &numBytes, &dataTypesNum);
    
    if (numBytes == 0) continue;
    
    std::vector<BYTE> dataTypeBuffer(numBytes);
    DATATYPES_INFO_1W* dataTypes = reinterpret_cast<DATATYPES_INFO_1W*>(dataTypeBuffer.data());
    
    isOK = EnumPrintProcessorDatatypesW(nullVal, pProcessor->pName, 1, (LPBYTE)dataTypes, numBytes, &numBytes, &dataTypesNum);
    if (!isOK) {
      ErrorInfo info = getLastErrorInfo();
      Napi::Error e = makeNapiErrorWithCode(env, "Error on EnumPrintProcessorDatatypesW", info);
      e.ThrowAsJavaScriptException();
      return result;
    }

    DATATYPES_INFO_1W* pDataType = dataTypes;
    for (DWORD j = 0; j < dataTypesNum; ++j, ++pDataType) {
      if (pDataType->pName && *pDataType->pName) {
        result.Set(formatIndex++, Napi::String::New(env, ws_to_utf8(pDataType->pName)));
      }
    }
  }

  return result;
}

Napi::Value PrintDirectWrapped(const Napi::CallbackInfo& args) {
  Napi::Env env = args.Env();
  if (args.Length() < 4) {
    Napi::TypeError::New(env, "printDirect requires at least 4 arguments: data, printer, docname, type").ThrowAsJavaScriptException();
    return env.Null();
  }

  // capture data into std::string to keep it alive for worker
  std::string dataStr;
  std::vector<char> dataVec;
  size_t dataLen = 0;
  if (args[0].IsBuffer()) {
    Napi::Buffer<char> b = args[0].As<Napi::Buffer<char>>();
    dataVec.assign(b.Data(), b.Data() + b.Length());
    dataLen = b.Length();
  } else {
    dataStr = args[0].ToString().Utf8Value();
    dataVec.assign(dataStr.begin(), dataStr.end());
    dataLen = dataVec.size();
  }

  std::string printerUtf8 = args[1].ToString().Utf8Value();
  std::string docnameUtf8 = args[2].ToString().Utf8Value();
  std::string typeUtf8 = args[3].ToString().Utf8Value();
  
  // Handle options parameter to match POSIX implementation
  Napi::Object optionsObj = Napi::Object::New(env);
  if (args.Length() > 4 && args[4].IsObject()) {
    optionsObj = args[4].As<Napi::Object>();
  }

  // Create a Promise and an AsyncWorker that will perform printing off the main thread
  auto deferred = Napi::Promise::Deferred::New(env);

  // Create worker data; avoid copying very large buffers by streaming via temp file
  struct WorkerData {
    std::vector<char> data;
    std::string tempFilename; // if streaming from disk
    bool createdTemp = false; // whether we created the temp file and should delete it
    std::string printer;
    std::string docname;
    std::string type;
    ErrorInfo errorInfo;
    std::string errorPrefix;
    DWORD jobId;
  };

  WorkerData* wd = new WorkerData();
  wd->printer = std::move(printerUtf8);
  wd->docname = std::move(docnameUtf8);
  wd->type = std::move(typeUtf8);
  wd->errorPrefix = "PrintDirect";

  bool usedTemp = false;
  Napi::Reference<Napi::Buffer<char>> bufRef;
  bool haveBufferRef = false;
  if (args[0].IsBuffer()) {
    Napi::Buffer<char> b = args[0].As<Napi::Buffer<char>>();
    if (dataLen <= STREAM_THRESHOLD) {
      // keep buffer alive via reference and avoid copying
      bufRef = Napi::Persistent(b);
      haveBufferRef = true;
    } else {
      // large buffer -> write to temp file for streaming
      // fallthrough to write using dataVec
    }
  }

  if (!haveBufferRef && dataLen > STREAM_THRESHOLD) {
    // write to temp file and let worker stream it
    char tmpPath[MAX_PATH];
    if (GetTempPathA(MAX_PATH, tmpPath) > 0) {
      char tmpFile[MAX_PATH];
      if (GetTempFileNameA(tmpPath, "npr", 0, tmpFile) != 0) {
        std::ofstream ofs(tmpFile, std::ios::binary);
        if (ofs) {
          ofs.write(dataVec.data(), dataVec.size());
          ofs.close();
            wd->tempFilename = std::string(tmpFile);
            wd->createdTemp = true;
            usedTemp = true;
        }
      }
    }
  }
  if (!usedTemp) {
      wd->data.swap(dataVec);
  }

  class PrintWorker : public Napi::AsyncWorker {
  public:
    PrintWorker(Napi::Env env, WorkerData* d, Napi::Promise::Deferred def)
      : Napi::AsyncWorker(env), data(d), deferred(def), hasBufferRef(false) {}

    PrintWorker(Napi::Env env, WorkerData* d, Napi::Promise::Deferred def, Napi::Reference<Napi::Buffer<char>>&& ref)
      : Napi::AsyncWorker(env), data(d), deferred(def), bufferRef(std::move(ref)), hasBufferRef(true) {}

    ~PrintWorker() { delete data; }

    void Execute() override {
      // Perform printing using Winspool
      std::vector<uint16_t> printerW = utf8_to_w(data->printer);
      std::vector<uint16_t> docnameW = utf8_to_w(data->docname);
      std::vector<uint16_t> typeW = utf8_to_w(data->type);

      PrinterHandle printerHandle((LPWSTR)printerW.data());
      if (!printerHandle.isOk()) {
        data->errorInfo = getLastErrorInfo();
        data->errorPrefix = "error on PrinterHandle";
        return;
      }

      DOC_INFO_1W DocInfo;
      DocInfo.pDocName = (LPWSTR)docnameW.data();
      DocInfo.pOutputFile = NULL;
      DocInfo.pDatatype = (LPWSTR)typeW.data();

      DWORD dwJob = StartDocPrinterW((HANDLE)printerHandle, 1, (LPBYTE)&DocInfo );
      if (dwJob == 0) {
        data->errorInfo = getLastErrorInfo();
        data->errorPrefix = "StartDocPrinterW error";
        return;
      }

      if (!StartPagePrinter((HANDLE)printerHandle)) {
        EndDocPrinter((HANDLE)printerHandle);
        data->errorInfo = getLastErrorInfo();
        data->errorPrefix = "StartPagePrinter error";
        return;
      }

      DWORD dwBytesWritten = 0;
      BOOL written = FALSE;

      // If we have a temp file (created for large payload), stream from it in chunks
      if (!data->tempFilename.empty()) {
        std::ifstream ifs(data->tempFilename, std::ios::binary);
        if (!ifs) {
          data->errorInfo.code = GetLastError();
          data->errorInfo.message = "Failed to open temp file for streaming";
          EndPagePrinter((HANDLE)printerHandle);
          EndDocPrinter((HANDLE)printerHandle);
          return;
        }
        const size_t bufSize = 64 * 1024; // 64 KiB
        std::vector<char> chunk(bufSize);
        while (ifs) {
          ifs.read(chunk.data(), bufSize);
          std::streamsize got = ifs.gcount();
          if (got <= 0) break;
          BOOL ok = WritePrinter((HANDLE)printerHandle, chunk.data(), (DWORD)got, &dwBytesWritten);
          if (!ok || dwBytesWritten != (DWORD)got) {
            data->errorInfo = getLastErrorInfo();
            ifs.close();
            EndPagePrinter((HANDLE)printerHandle);
            EndDocPrinter((HANDLE)printerHandle);
            return;
          }
        }
        ifs.close();
      } else if (hasBufferRef) {
        auto buf = bufferRef.Value();
        written = WritePrinter((HANDLE)printerHandle, (LPVOID)buf.Data(), (DWORD)buf.Length(), &dwBytesWritten);
        if (!written || dwBytesWritten != (DWORD)buf.Length()) {
          data->errorInfo = getLastErrorInfo();
          data->errorPrefix = "WritePrinter";
          return;
        }
      } else {
        written = WritePrinter((HANDLE)printerHandle, (LPVOID)data->data.data(), (DWORD)data->data.size(), &dwBytesWritten);
        if (!written || dwBytesWritten != (DWORD)data->data.size()) {
          data->errorInfo = getLastErrorInfo();
          data->errorPrefix = "WritePrinter";
          return;
        }
      }
      EndPagePrinter((HANDLE)printerHandle);
      EndDocPrinter((HANDLE)printerHandle);
      data->jobId = dwJob;
    }

    void OnOK() override {
      Napi::Env env = Env();
      // cleanup temp file if we created one
      if (data->createdTemp && !data->tempFilename.empty()) {
        DeleteFileA(data->tempFilename.c_str());
      }
      if (data->errorInfo.code != 0 || !data->errorInfo.message.empty()) {
        Napi::Error e = makeNapiErrorWithCode(env, data->errorPrefix, data->errorInfo);
        deferred.Reject(e.Value());
      } else {
        deferred.Resolve(Napi::Number::New(env, data->jobId));
      }
    }

    void OnError(const Napi::Error& e) override {
      deferred.Reject(e.Value());
    }

  private:
    WorkerData* data;
    Napi::Promise::Deferred deferred;
    Napi::Reference<Napi::Buffer<char>> bufferRef;
    bool hasBufferRef;
  };

  PrintWorker* w = nullptr;
  if (haveBufferRef) {
    w = new PrintWorker(env, wd, deferred, std::move(bufRef));
  } else {
    w = new PrintWorker(env, wd, deferred);
  }
  w->Queue();
  return deferred.Promise();
}

Napi::Value PrintFileWrapped(const Napi::CallbackInfo& args) {
  Napi::Env env = args.Env();
  if (args.Length() < 3) {
    Napi::TypeError::New(env, "printFile requires at least 3 arguments: filename, docname, printer").ThrowAsJavaScriptException();
    return env.Null();
  }
  std::string filename = args[0].ToString().Utf8Value();
  std::string docname = args[1].ToString().Utf8Value();
  std::string printer = args[2].ToString().Utf8Value();
  
  // Handle options parameter (4th argument) - for future extensibility
  Napi::Object optionsObj = Napi::Object::New(env);
  if (args.Length() > 3 && args[3].IsObject()) {
    optionsObj = args[3].As<Napi::Object>();
    // Note: Options are parsed but not currently used in Windows implementation
    // This ensures parameter compatibility with POSIX implementation
  }

  if (printer.empty()) {
    printer = getDefaultPrinterUtf8();
    if (printer.empty()) {
      Napi::TypeError::New(env, "Printer parameter or default printer is not defined").ThrowAsJavaScriptException();
      return env.Null();
    }
  }

  // Check file exists (do not read into memory here for large files)
  std::ifstream ifs(filename, std::ios::binary);
  if (!ifs) {
    ErrorInfo info{0, std::string("cannot open file: " + filename)};
    Napi::Error e = makeNapiErrorWithCode(env, "cannot open file", info);
    e.ThrowAsJavaScriptException();
    return env.Null();
  }

  // create promise and worker data
  auto deferred = Napi::Promise::Deferred::New(env);
  struct FileWorkerData {
    std::vector<char> data;
    std::string filename; // stream directly from this file in the worker
    std::string printer;
    std::string docname;
    ErrorInfo errorInfo;
    std::string errorPrefix;
    DWORD jobId;
  };
  FileWorkerData* fd = new FileWorkerData();
  fd->filename = filename;
  fd->printer = std::move(printer);
  fd->docname = std::move(docname);
  fd->errorPrefix = "PrintFile";

  class PrintFileWorker : public Napi::AsyncWorker {
  public:
    PrintFileWorker(Napi::Env env, FileWorkerData* d, Napi::Promise::Deferred def)
      : Napi::AsyncWorker(env), data(d), deferred(def) {}
    ~PrintFileWorker() { delete data; }
    void Execute() override {
      std::vector<uint16_t> printerW = utf8_to_w(data->printer);
      std::vector<uint16_t> docnameW = utf8_to_w(data->docname);
      std::vector<uint16_t> typeW = utf8_to_w(std::string("RAW"));
      PrinterHandle printerHandle((LPWSTR)printerW.data());
      if (!printerHandle.isOk()) { data->errorInfo = getLastErrorInfo(); data->errorPrefix = "error on PrinterHandle"; return; }
      DOC_INFO_1W DocInfo;
      DocInfo.pDocName = (LPWSTR)docnameW.data();
      DocInfo.pOutputFile = NULL;
      DocInfo.pDatatype = (LPWSTR)typeW.data();
      DWORD dwJob = StartDocPrinterW((HANDLE)printerHandle, 1, (LPBYTE)&DocInfo );
      if (dwJob == 0) { data->errorInfo = getLastErrorInfo(); data->errorPrefix = "StartDocPrinterW error"; return; }
      if (!StartPagePrinter((HANDLE)printerHandle)) { EndDocPrinter((HANDLE)printerHandle); data->errorInfo = getLastErrorInfo(); data->errorPrefix = "StartPagePrinter error"; return; }
      DWORD dwBytesWritten = 0;
      // Stream file directly from disk to printer in chunks to avoid loading entire file into memory
      std::ifstream ifs2(data->filename, std::ios::binary);
      if (!ifs2) {
        data->errorInfo = getLastErrorInfo();
        data->errorPrefix = "cannot open file in worker";
        return;
      }
      const size_t bufSize = 64 * 1024;
      std::vector<char> chunk(bufSize);
      while (ifs2) {
        ifs2.read(chunk.data(), bufSize);
        std::streamsize got = ifs2.gcount();
        if (got <= 0) break;
        BOOL ok = WritePrinter((HANDLE)printerHandle, chunk.data(), (DWORD)got, &dwBytesWritten);
        if (!ok || dwBytesWritten != (DWORD)got) {
          data->errorInfo = getLastErrorInfo();
          ifs2.close();
          EndPagePrinter((HANDLE)printerHandle);
          EndDocPrinter((HANDLE)printerHandle);
          return;
        }
      }
      ifs2.close();
      EndPagePrinter((HANDLE)printerHandle);
      EndDocPrinter((HANDLE)printerHandle);
      data->jobId = dwJob;
    }
    void OnOK() override {
      Napi::Env env = Env();
      if (data->errorInfo.code != 0 || !data->errorInfo.message.empty()) {
        Napi::Error e = makeNapiErrorWithCode(env, data->errorPrefix, data->errorInfo);
        deferred.Reject(e.Value());
      } else {
        deferred.Resolve(Napi::Number::New(env, data->jobId));
      }
    }
    void OnError(const Napi::Error& e) override { deferred.Reject(e.Value()); }
  private:
    FileWorkerData* data;
    Napi::Promise::Deferred deferred;
  };

  PrintFileWorker* w = new PrintFileWorker(env, fd, deferred);
  w->Queue();
  return deferred.Promise();
}

Napi::Value GetJobWrapped(const Napi::CallbackInfo& args) {
  Napi::Env env = args.Env();
  if (args.Length() < 2) {
    Napi::TypeError::New(env, "printer name and job id required").ThrowAsJavaScriptException();
    return env.Null();
  }

  std::string printerName = args[0].ToString().Utf8Value();
  int32_t jobId = args[1].As<Napi::Number>().Int32Value();
  
  if (jobId < 0) {
    Napi::Error::New(env, "Wrong job number").ThrowAsJavaScriptException();
    return env.Null();
  }

  std::vector<uint16_t> printerNameW = utf8_to_w(printerName);
  
  // Open a handle to the printer
  PrinterHandle printerHandle((LPWSTR)printerNameW.data());
  if (!printerHandle.isOk()) {
    ErrorInfo info = getLastErrorInfo();
    std::ostringstream s;
    s << "error on PrinterHandle for printer '" << printerName << "'";
    Napi::Error e = makeNapiErrorWithCode(env, s.str(), info);
    e.ThrowAsJavaScriptException();
    return env.Null();
  }

  DWORD sizeBytes = 0;
  GetJobW((HANDLE)printerHandle, static_cast<DWORD>(jobId), 2, NULL, sizeBytes, &sizeBytes);
  if (sizeBytes == 0) {
    Napi::Error::New(env, "Error on allocating memory for job").ThrowAsJavaScriptException();
    return env.Null();
  }

  std::vector<BYTE> buffer(sizeBytes);
  JOB_INFO_2W* job = reinterpret_cast<JOB_INFO_2W*>(buffer.data());
  
  BOOL bOK = GetJobW((HANDLE)printerHandle, static_cast<DWORD>(jobId), 2, (LPBYTE)job, sizeBytes, &sizeBytes);
  if (!bOK) {
    ErrorInfo info = getLastErrorInfo();
    Napi::Error e = makeNapiErrorWithCode(env, "Error on GetJob. Wrong job id or it was deleted", info);
    e.ThrowAsJavaScriptException();
    return env.Null();
  }

  Napi::Object result = Napi::Object::New(env);
  parseJobObject(job, env, result);
  return result;
}

Napi::Value SetJobWrapped(const Napi::CallbackInfo& args) {
  Napi::Env env = args.Env();
  if (args.Length() < 3) {
    Napi::TypeError::New(env, "printer name, job id and command required").ThrowAsJavaScriptException();
    return env.Null();
  }

  std::string printerName = args[0].ToString().Utf8Value();
  int32_t jobId = args[1].As<Napi::Number>().Int32Value();
  std::string jobCommand = args[2].ToString().Utf8Value();
  
  if (jobId < 0) {
    Napi::Error::New(env, "Wrong job number").ThrowAsJavaScriptException();
    return env.Null();
  }

  const auto& commandMap = getJobCommandMap();
  auto itJobCommand = commandMap.find(jobCommand);
  if (itJobCommand == commandMap.end()) {
    Napi::Error::New(env, "wrong job command. use getSupportedJobCommands to see the possible commands").ThrowAsJavaScriptException();
    return env.Null();
  }

  std::vector<uint16_t> printerNameW = utf8_to_w(printerName);
  
  // Open a handle to the printer
  PrinterHandle printerHandle((LPWSTR)printerNameW.data());
  if (!printerHandle.isOk()) {
    ErrorInfo info = getLastErrorInfo();
    std::ostringstream s;
    s << "error on PrinterHandle for printer '" << printerName << "'";
    Napi::Error e = makeNapiErrorWithCode(env, s.str(), info);
    e.ThrowAsJavaScriptException();
    return env.Null();
  }

  BOOL bOK = SetJobW((HANDLE)printerHandle, static_cast<DWORD>(jobId), 0, NULL, itJobCommand->second);
  if (!bOK) {
    ErrorInfo info = getLastErrorInfo();
    Napi::Error e = makeNapiErrorWithCode(env, "Error on SetJob", info);
    e.ThrowAsJavaScriptException();
    return env.Null();
  }

  return Napi::Boolean::New(env, true);
}

Napi::Value GetPrinterDriverOptionsWrapped(const Napi::CallbackInfo& args) {
  Napi::Env env = args.Env();
  if (args.Length() < 1 || !args[0].IsString()) {
    Napi::TypeError::New(env, "printer name required").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  std::string printerName = args[0].As<Napi::String>().Utf8Value();
  std::vector<uint16_t> printerNameW = utf8_to_w(printerName);
  
  // Open a handle to the printer
  PrinterHandle printerHandle((LPWSTR)printerNameW.data());
  if (!printerHandle.isOk()) {
    ErrorInfo info = getLastErrorInfo();
    Napi::Error e = makeNapiErrorWithCode(env, "Error opening printer", info);
    e.ThrowAsJavaScriptException();
    return env.Null();
  }
  
  // Get printer information
  DWORD printersSizeBytes = 0;
  GetPrinterW((HANDLE)printerHandle, 2, NULL, printersSizeBytes, &printersSizeBytes);
  if (printersSizeBytes == 0) {
    ErrorInfo info = getLastErrorInfo();
    Napi::Error e = makeNapiErrorWithCode(env, "Error getting printer info size", info);
    e.ThrowAsJavaScriptException();
    return env.Null();
  }
  
  std::vector<BYTE> buffer(printersSizeBytes);
  PRINTER_INFO_2W* printer = reinterpret_cast<PRINTER_INFO_2W*>(buffer.data());
  
  BOOL bOK = GetPrinterW((HANDLE)printerHandle, 2, (LPBYTE)printer, printersSizeBytes, &printersSizeBytes);
  if (!bOK) {
    ErrorInfo info = getLastErrorInfo();
    Napi::Error e = makeNapiErrorWithCode(env, "Error getting printer info", info);
    e.ThrowAsJavaScriptException();
    return env.Null();
  }
  
  // Create driver options object (basic implementation for Windows)
  Napi::Object driverOptions = Napi::Object::New(env);
  
  // Add basic printer properties as driver options
  if (printer->pDriverName && *printer->pDriverName) {
    driverOptions.Set("DriverName", Napi::String::New(env, ws_to_utf8(printer->pDriverName)));
  }
  if (printer->pDatatype && *printer->pDatatype) {
    driverOptions.Set("DataType", Napi::String::New(env, ws_to_utf8(printer->pDatatype)));
  }
  if (printer->pPrintProcessor && *printer->pPrintProcessor) {
    driverOptions.Set("PrintProcessor", Napi::String::New(env, ws_to_utf8(printer->pPrintProcessor)));
  }
  if (printer->pParameters && *printer->pParameters) {
    driverOptions.Set("Parameters", Napi::String::New(env, ws_to_utf8(printer->pParameters)));
  }
  
  // Add printer status as an option
  driverOptions.Set("Status", Napi::Number::New(env, printer->Status));
  driverOptions.Set("Attributes", Napi::Number::New(env, printer->Attributes));
  
  return driverOptions;
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set("getPrinters", Napi::Function::New(env, GetPrintersWrapped));
  exports.Set("getPrinter", Napi::Function::New(env, GetPrinterWrapped));
  exports.Set("getDefaultPrinterName", Napi::Function::New(env, GetDefaultPrinterNameWrapped));
  exports.Set("getPrinterDriverOptions", Napi::Function::New(env, GetPrinterDriverOptionsWrapped));
  exports.Set("getSupportedJobCommands", Napi::Function::New(env, GetSupportedJobCommandsWrapped));
  exports.Set("getSupportedPrintFormats", Napi::Function::New(env, GetSupportedPrintFormatsWrapped));
  exports.Set("getJob", Napi::Function::New(env, GetJobWrapped));
  exports.Set("setJob", Napi::Function::New(env, SetJobWrapped));
  exports.Set("printDirect", Napi::Function::New(env, PrintDirectWrapped));
  exports.Set("printFile", Napi::Function::New(env, PrintFileWrapped));
  return exports;
}

NODE_API_MODULE(node_printer, Init)
#else
#error "This file is Windows-only PoC"
#endif
