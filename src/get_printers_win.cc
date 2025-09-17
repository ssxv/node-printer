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

static std::string getLastErrorMessage() {
  std::ostringstream s;
  DWORD code = GetLastError();
  s << "code: " << code;
  LPWSTR buf = NULL;
  DWORD len = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                            NULL, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&buf, 0, NULL);
  if (len && buf) {
    s << ", message: " << ws_to_utf8(buf);
    LocalFree(buf);
  }
  return s.str();
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

// Forward declarations
static double systemtime_to_epoch_seconds(const SYSTEMTIME &st);
static void parseJobObject_add_times(const JOB_INFO_2W* job, Napi::Env env, Napi::Object& out);

static void retrieveAndParseJobs(LPWSTR printerName, DWORD totalJobs, Napi::Env env, Napi::Array& jobsOut) {
  if (totalJobs == 0) return;
  PrinterHandle handle(printerName);
  if (!handle.isOk()) {
    Napi::Object err = Napi::Object::New(env);
    err.Set("error", Napi::String::New(env, std::string("error on PrinterHandle: ") + getLastErrorMessage()));
    jobsOut.Set((uint32_t)0, err);
    return;
  }

  DWORD bytesNeeded = 0, returned = 0;
  BOOL b = EnumJobsW(handle, 0, totalJobs, 2, NULL, 0, &bytesNeeded, &returned);
  std::vector<BYTE> buffer(bytesNeeded);
  b = EnumJobsW(handle, 0, totalJobs, 2, buffer.data(), bytesNeeded, &bytesNeeded, &returned);
  if (!b) {
    Napi::Object err = Napi::Object::New(env);
    err.Set("error", Napi::String::New(env, std::string("Error on EnumJobsW: ") + getLastErrorMessage()));
    jobsOut.Set((uint32_t)0, err);
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
    Napi::TypeError::New(env, std::string("Error on EnumPrinters: ") + getLastErrorMessage()).ThrowAsJavaScriptException();
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

  // Create a Promise and an AsyncWorker that will perform printing off the main thread
  auto deferred = Napi::Promise::Deferred::New(env);

  // Create a copy of data for the worker
  struct WorkerData {
    std::vector<char> data;
    std::string printer;
    std::string docname;
    std::string type;
    std::string error;
    DWORD jobId;
  };

  WorkerData* wd = new WorkerData();
  wd->data.swap(dataVec);
  wd->printer = std::move(printerUtf8);
  wd->docname = std::move(docnameUtf8);
  wd->type = std::move(typeUtf8);

  class PrintWorker : public Napi::AsyncWorker {
  public:
    PrintWorker(Napi::Env env, WorkerData* d, Napi::Promise::Deferred def)
      : Napi::AsyncWorker(env), data(d), deferred(def) {}

    ~PrintWorker() { delete data; }

    void Execute() override {
      // Perform printing using Winspool
      std::vector<uint16_t> printerW = utf8_to_w(data->printer);
      std::vector<uint16_t> docnameW = utf8_to_w(data->docname);
      std::vector<uint16_t> typeW = utf8_to_w(data->type);

      PrinterHandle printerHandle((LPWSTR)printerW.data());
      if (!printerHandle.isOk()) {
        data->error = std::string("error on PrinterHandle: ") + getLastErrorMessage();
        return;
      }

      DOC_INFO_1W DocInfo;
      DocInfo.pDocName = (LPWSTR)docnameW.data();
      DocInfo.pOutputFile = NULL;
      DocInfo.pDatatype = (LPWSTR)typeW.data();

      DWORD dwJob = StartDocPrinterW((HANDLE)printerHandle, 1, (LPBYTE)&DocInfo );
      if (dwJob == 0) {
        data->error = std::string("StartDocPrinterW error: ") + getLastErrorMessage();
        return;
      }

      if (!StartPagePrinter((HANDLE)printerHandle)) {
        EndDocPrinter((HANDLE)printerHandle);
        data->error = std::string("StartPagePrinter error: ") + getLastErrorMessage();
        return;
      }

      DWORD dwBytesWritten = 0;
      BOOL written = WritePrinter((HANDLE)printerHandle, (LPVOID)data->data.data(), (DWORD)data->data.size(), &dwBytesWritten);
      EndPagePrinter((HANDLE)printerHandle);
      EndDocPrinter((HANDLE)printerHandle);
      if (!written) {
        data->error = std::string("WritePrinter error: ") + getLastErrorMessage();
        return;
      }
      if (dwBytesWritten != data->data.size()) {
        data->error = "not sent all bytes";
        return;
      }
      data->jobId = dwJob;
    }

    void OnOK() override {
      Napi::Env env = Env();
      if (!data->error.empty()) {
        deferred.Reject(Napi::Error::New(env, data->error).Value());
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
  };

  PrintWorker* w = new PrintWorker(env, wd, deferred);
  w->Queue();
  return deferred.Promise();
}

Napi::Value PrintFileWrapped(const Napi::CallbackInfo& args) {
  Napi::Env env = args.Env();
  if (args.Length() < 1) {
    Napi::TypeError::New(env, "printFile requires at least 1 argument: filename").ThrowAsJavaScriptException();
    return env.Null();
  }
  std::string filename = args[0].ToString().Utf8Value();
  std::string docname = (args.Length() > 1 && !args[1].IsUndefined()) ? args[1].ToString().Utf8Value() : filename;
  std::string printer = (args.Length() > 2 && !args[2].IsUndefined()) ? args[2].ToString().Utf8Value() : std::string();

  if (printer.empty()) {
    printer = getDefaultPrinterUtf8();
    if (printer.empty()) {
      Napi::TypeError::New(env, "Printer parameter or default printer is not defined").ThrowAsJavaScriptException();
      return env.Null();
    }
  }

  // read file into vector
  std::ifstream ifs(filename, std::ios::binary);
  if (!ifs) {
    Napi::TypeError::New(env, std::string("cannot open file: ") + filename).ThrowAsJavaScriptException();
    return env.Null();
  }
  std::vector<char> contents((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

  // create promise and worker data
  auto deferred = Napi::Promise::Deferred::New(env);
  struct FileWorkerData {
    std::vector<char> data;
    std::string printer;
    std::string docname;
    std::string error;
    DWORD jobId;
  };
  FileWorkerData* fd = new FileWorkerData();
  fd->data.swap(contents);
  fd->printer = std::move(printer);
  fd->docname = std::move(docname);

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
      if (!printerHandle.isOk()) { data->error = std::string("error on PrinterHandle: ") + getLastErrorMessage(); return; }
      DOC_INFO_1W DocInfo;
      DocInfo.pDocName = (LPWSTR)docnameW.data();
      DocInfo.pOutputFile = NULL;
      DocInfo.pDatatype = (LPWSTR)typeW.data();
      DWORD dwJob = StartDocPrinterW((HANDLE)printerHandle, 1, (LPBYTE)&DocInfo );
      if (dwJob == 0) { data->error = std::string("StartDocPrinterW error: ") + getLastErrorMessage(); return; }
      if (!StartPagePrinter((HANDLE)printerHandle)) { EndDocPrinter((HANDLE)printerHandle); data->error = std::string("StartPagePrinter error: ") + getLastErrorMessage(); return; }
      DWORD dwBytesWritten = 0;
      BOOL written = WritePrinter((HANDLE)printerHandle, (LPVOID)data->data.data(), (DWORD)data->data.size(), &dwBytesWritten);
      EndPagePrinter((HANDLE)printerHandle);
      EndDocPrinter((HANDLE)printerHandle);
      if (!written) { data->error = std::string("WritePrinter error: ") + getLastErrorMessage(); return; }
      if (dwBytesWritten != data->data.size()) { data->error = "not sent all bytes"; return; }
      data->jobId = dwJob;
    }
    void OnOK() override {
      Napi::Env env = Env();
      if (!data->error.empty()) deferred.Reject(Napi::Error::New(env, data->error).Value()); else deferred.Resolve(Napi::Number::New(env, data->jobId));
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

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set("getPrinters", Napi::Function::New(env, GetPrintersWrapped));
  exports.Set("printDirect", Napi::Function::New(env, PrintDirectWrapped));
  exports.Set("printFile", Napi::Function::New(env, PrintFileWrapped));
  return exports;
}

NODE_API_MODULE(node_printer, Init)
#else
#error "This file is Windows-only PoC"
#endif
