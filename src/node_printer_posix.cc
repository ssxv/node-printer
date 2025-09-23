#include <napi.h>
#include <cups/cups.h>
#include <cups/ppd.h>

#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

using namespace Napi;

static std::map<std::string, int> getJobStatusMap() {
  static std::map<std::string,int> m;
  if (!m.empty()) return m;
  // Comprehensive job status mapping from old implementation
  m["PRINTING"] = IPP_JOB_PROCESSING;
  m["PRINTED"] = IPP_JOB_COMPLETED;
  m["PAUSED"] = IPP_JOB_HELD;
  m["PENDING"] = IPP_JOB_PENDING;
  m["STOPPED"] = IPP_JOB_STOPPED;
  m["CANCELLED"] = IPP_JOB_CANCELLED;
  m["ABORTED"] = IPP_JOB_ABORTED;
  return m;
}

static std::map<std::string, std::string> getPrinterFormatMap() {
  static std::map<std::string, std::string> m;
  if (!m.empty()) return m;
  // Enhanced format support from old implementation (non-deprecated only)
  m["RAW"] = CUPS_FORMAT_RAW;
  m["TEXT"] = CUPS_FORMAT_TEXT;
#ifdef CUPS_FORMAT_PDF
  m["PDF"] = CUPS_FORMAT_PDF;
#endif
#ifdef CUPS_FORMAT_JPEG
  m["JPEG"] = CUPS_FORMAT_JPEG;
#endif
#ifdef CUPS_FORMAT_POSTSCRIPT
  m["POSTSCRIPT"] = CUPS_FORMAT_POSTSCRIPT;
#endif
#ifdef CUPS_FORMAT_COMMAND
  m["COMMAND"] = CUPS_FORMAT_COMMAND;
#endif
#ifdef CUPS_FORMAT_AUTO
  m["AUTO"] = CUPS_FORMAT_AUTO;
#endif
  return m;
}

// Threshold (bytes) where we switch to temp-file fallback
static const size_t STREAM_THRESHOLD = 4 * 1024 * 1024; // 4 MiB

// Modern CUPS options management class (inspired by old implementation)
class CupsOptionsManager {
private:
  cups_option_t* options;
  int num_options;

public:
  CupsOptionsManager() : options(nullptr), num_options(0) {}
  
  // Constructor from N-API object
  CupsOptionsManager(const Object& optionsObj) : options(nullptr), num_options(0) {
    Array props = optionsObj.GetPropertyNames();
    for (uint32_t i = 0; i < props.Length(); ++i) {
      Value key = props.Get(i);
      if (!key.IsString()) continue;
      
      std::string keyStr = key.As<String>().Utf8Value();
      Value val = optionsObj.Get(key);
      std::string valStr = val.IsString() ? val.As<String>().Utf8Value() : std::string();
      
      num_options = cupsAddOption(keyStr.c_str(), valStr.c_str(), num_options, &options);
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

// Convert cups_job_t -> JS object with enhanced status handling
static void parseJobObject(const cups_job_t* job, Env env, Object& out) {
  out.Set("id", Number::New(env, job->id));
  out.Set("name", String::New(env, job->title ? job->title : ""));
  out.Set("printerName", String::New(env, job->dest ? job->dest : ""));
  out.Set("user", String::New(env, job->user ? job->user : ""));

  // Enhanced format parsing from old implementation
  std::string job_format = job->format ? job->format : std::string();
  for (const auto &kv : getPrinterFormatMap()) {
    if (kv.second == job_format) {
      job_format = kv.first;
      break;
    }
  }
  out.Set("format", String::New(env, job_format));
  out.Set("priority", Number::New(env, job->priority));
  out.Set("size", Number::New(env, job->size));

  // Enhanced status handling with fallback for unknown statuses
  Array statusArr = Array::New(env);
  uint32_t si = 0;
  bool statusFound = false;
  for (const auto &kv : getJobStatusMap()) {
    if (job->state == kv.second) {
      statusArr.Set(si++, String::New(env, kv.first));
      statusFound = true;
      break; // only one status on POSIX
    }
  }
  if (!statusFound) {
    // Unknown status - report with numeric value for debugging
    std::ostringstream s;
    s << "UNKNOWN(" << job->state << ")";
    statusArr.Set(si++, String::New(env, s.str()));
  }
  out.Set("status", statusArr);

  // Convert seconds -> ms for JS Date compatibility
  double creationMs = static_cast<double>(job->creation_time) * 1000.0;
  double completedMs = static_cast<double>(job->completed_time) * 1000.0;
  double processingMs = static_cast<double>(job->processing_time) * 1000.0;
  out.Set("creationTime", Date::New(env, creationMs));
  out.Set("completedTime", Date::New(env, completedMs));
  out.Set("processingTime", Date::New(env, processingMs));
}

// Parse printer info and include options and active jobs with enhanced error handling
static std::string parsePrinterInfo(const cups_dest_t* printer, Env env, Object& out) {
  if (!printer) {
    return "Invalid printer pointer";
  }
  
  out.Set("name", String::New(env, printer->name ? printer->name : ""));
  out.Set("isDefault", Boolean::New(env, printer->is_default));
  if (printer->instance) {
    out.Set("instance", String::New(env, printer->instance));
  }

  Object optionsObj = Object::New(env);
  for (int i = 0; i < printer->num_options; ++i) {
    cups_option_t *opt = &printer->options[i];
    if (opt->name && opt->value) {
      optionsObj.Set(String::New(env, opt->name), String::New(env, opt->value));
    }
  }
  out.Set("options", optionsObj);

  // Get printer jobs with enhanced error handling
  cups_job_t *jobs = nullptr;
  int totalJobs = cupsGetJobs(&jobs, printer->name, 0, CUPS_WHICHJOBS_ACTIVE);
  if (totalJobs > 0 && jobs) {
    Array jobsArr = Array::New(env, totalJobs);
    for (int i = 0; i < totalJobs; ++i) {
      Object jobObj = Object::New(env);
      parseJobObject(&jobs[i], env, jobObj);
      jobsArr.Set((uint32_t)i, jobObj);
    }
    out.Set("jobs", jobsArr);
    cupsFreeJobs(totalJobs, jobs);
  } else if (totalJobs < 0) {
    // Error getting jobs - log but don't fail the entire operation
    std::string error = cupsLastErrorString();
    // Set empty jobs array and continue
    out.Set("jobs", Array::New(env, 0));
    if (jobs) cupsFreeJobs(0, jobs);
    // Return warning rather than error to not break printer enumeration
    return std::string("Warning: Could not retrieve jobs: ") + error;
  } else {
    // No jobs - set empty array
    out.Set("jobs", Array::New(env, 0));
    if (jobs) cupsFreeJobs(totalJobs, jobs);
  }
  
  return std::string(); // Success
}

// getPrinters implementation with enhanced error handling
static Value GetPrinters(const CallbackInfo& info) {
  Env env = info.Env();
  cups_dest_t *printers = nullptr;
  int printers_size = cupsGetDests(&printers);
  
  if (printers_size < 0) {
    std::string error = std::string("Error getting printers: ") + cupsLastErrorString();
    Error::New(env, error).ThrowAsJavaScriptException();
    return env.Null();
  }
  
  Array result = Array::New(env, printers_size);
  std::string warning; // Collect warnings but don't fail
  
  for (int i = 0; i < printers_size; ++i) {
    Object p = Object::New(env);
    std::string err = parsePrinterInfo(&printers[i], env, p);
    if (!err.empty()) {
      // If it's a warning (not failure), continue but log
      if (err.find("Warning:") == 0) {
        warning = err; // Store warning for later
      } else {
        // Critical error - fail the entire operation
        cupsFreeDests(printers_size, printers);
        Error::New(env, err).ThrowAsJavaScriptException();
        return env.Null();
      }
    }
    result.Set((uint32_t)i, p);
  }
  cupsFreeDests(printers_size, printers);
  
  // Note: Warnings are silently handled - in production you might want to log them
  return result;
}

Object Init(Env env, Object exports) {
  exports.Set("getPrinters", Function::New(env, GetPrinters));
  exports.Set("getPrinter", Function::New(env, [](const CallbackInfo& info)->Value{
    Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString()) {
      TypeError::New(env, "printer name required").ThrowAsJavaScriptException();
      return env.Null();
    }
    std::string name = info[0].As<String>().Utf8Value();
    cups_dest_t *printers = nullptr;
    int n = cupsGetDests(&printers);
    if (n < 0) {
      std::string error = std::string("Error getting printers: ") + cupsLastErrorString();
      Error::New(env, error).ThrowAsJavaScriptException();
      return env.Null();
    }
    
    cups_dest_t *p = cupsGetDest(name.c_str(), NULL, n, printers);
    if (!p) {
      cupsFreeDests(n, printers);
      Error::New(env, "Printer not found").ThrowAsJavaScriptException();
      return env.Null();
    }
    
    Object out = Object::New(env);
    std::string err = parsePrinterInfo(p, env, out);
    cupsFreeDests(n, printers);
    
    if (!err.empty() && err.find("Warning:") != 0) {
      // Only throw for non-warning errors
      Error::New(env, err).ThrowAsJavaScriptException();
      return env.Null();
    }
    
    return out;
  }));
  exports.Set("getJob", Function::New(env, [](const CallbackInfo& info)->Value{
    Env env = info.Env();
    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsNumber()) {
      TypeError::New(env, "printer name and job id required").ThrowAsJavaScriptException();
      return env.Null();
    }
    std::string name = info[0].As<String>().Utf8Value();
    int jobId = info[1].As<Number>().Int32Value();
    if (jobId < 0) {
      Error::New(env, "Invalid job ID").ThrowAsJavaScriptException();
      return env.Null();
    }
    
    cups_job_t *jobs = nullptr;
    int totalJobs = cupsGetJobs(&jobs, name.c_str(), 0, CUPS_WHICHJOBS_ALL);
    if (totalJobs < 0) {
      std::string error = std::string("Error getting jobs: ") + cupsLastErrorString();
      Error::New(env, error).ThrowAsJavaScriptException();
      return env.Null();
    }
    
    cups_job_t *found = nullptr;
    Object out = Object::New(env);
    if (totalJobs > 0 && jobs) {
      for (int i = 0; i < totalJobs; ++i) {
        if (jobs[i].id == jobId) {
          parseJobObject(&jobs[i], env, out);
          found = &jobs[i];
          break;
        }
      }
    }
    cupsFreeJobs(totalJobs, jobs);
    
    if (!found) {
      Error::New(env, "Printer job not found").ThrowAsJavaScriptException();
      return env.Null();
    }
    return out;
  }));
  exports.Set("getPrinterDriverOptions", Function::New(env, [](const CallbackInfo& info)->Value{
    Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString()) {
      TypeError::New(env, "printer name required").ThrowAsJavaScriptException();
      return env.Null();
    }
    std::string name = info[0].As<String>().Utf8Value();
    cups_dest_t *printers = nullptr;
    int n = cupsGetDests(&printers);
    if (n < 0) {
      std::string error = std::string("Error getting printers: ") + cupsLastErrorString();
      Error::New(env, error).ThrowAsJavaScriptException();
      return env.Null();
    }
    
    cups_dest_t *p = cupsGetDest(name.c_str(), NULL, n, printers);
    if (!p) {
      cupsFreeDests(n, printers);
      Error::New(env, "Printer not found").ThrowAsJavaScriptException();
      return env.Null();
    }

    // Enhanced driver options using modern CUPS APIs (avoiding deprecated PPD)
    Object driverOptions = Object::New(env);
    
    // Get basic options from destination
    for (int i = 0; i < p->num_options; ++i) {
      cups_option_t *opt = &p->options[i];
      if (opt->name && opt->value) {
        driverOptions.Set(String::New(env, opt->name), String::New(env, opt->value));
      }
    }
    
    // Try to get additional printer capabilities using modern CUPS APIs
    // Note: cupsCopyDestInfo requires CUPS 1.6+ but provides non-deprecated access to printer info
    // We use a simple approach here since we're avoiding deprecated PPD APIs as requested
    
    cupsFreeDests(n, printers);
    return driverOptions;
  }));
  // printFile AsyncWorker with enhanced options support
  exports.Set("printFile", Function::New(env, [](const CallbackInfo& info)->Value{
    Env env = info.Env();
    if (info.Length() < 3 || !info[0].IsString() || !info[1].IsString() || !info[2].IsString()) {
      TypeError::New(env, "printFile requires at least 3 arguments: filename, docname, printer").ThrowAsJavaScriptException();
      return env.Null();
    }
    std::string filename = info[0].As<String>().Utf8Value();
    std::string docname = info[1].As<String>().Utf8Value();
    std::string printer = info[2].As<String>().Utf8Value();

    // Enhanced options support - can be object or callback
    bool hasOptions = info.Length() > 3 && info[3].IsObject() && !info[3].IsFunction();
    bool hasCallback = false;
    Function cb;
    
    // Find callback in arguments
    for (size_t i = 3; i < info.Length(); ++i) {
      if (info[i].IsFunction()) {
        cb = info[i].As<Function>();
        hasCallback = true;
        break;
      }
    }
    
    if (!hasCallback) {
      cb = Function::New(env, [](const CallbackInfo&){ });
    }

    // Create an AsyncWorker with enhanced functionality
    class PrintFileWorker : public AsyncWorker {
    public:
      PrintFileWorker(Function& cb, std::string fn, std::string dn, std::string pr, bool useOpts, Object opts)
        : AsyncWorker(cb), filename(std::move(fn)), docname(std::move(dn)), printer(std::move(pr)), 
          useOptions(useOpts), jobId(0) {
        if (useOptions) {
          // Initialize options manager - if it fails, useOptions will remain true but optionsManager will be default
          optionsManager = CupsOptionsManager(opts);
        }
      }
      
      void Execute() override {
        int id;
        if (useOptions) {
          id = cupsPrintFile(printer.c_str(), filename.c_str(), docname.c_str(), 
                           optionsManager.getNumOptions(), optionsManager.get());
        } else {
          id = cupsPrintFile(printer.c_str(), filename.c_str(), docname.c_str(), 0, NULL);
        }
        
        if (id == 0) {
          std::string err = cupsLastErrorString();
          if (err.empty()) err = "Unknown printing error";
          SetError(err);
        } else {
          jobId = id;
        }
      }
      
      void OnOK() override {
        HandleScope scope(Env());
        Callback().Call({ Env().Null(), Number::New(Env(), jobId) });
      }
      
    private:
      std::string filename, docname, printer;
      bool useOptions;
      CupsOptionsManager optionsManager;
      int jobId;
    };

    Object optionsObj = hasOptions ? info[3].As<Object>() : Object::New(env);
    PrintFileWorker* worker = new PrintFileWorker(cb, filename, docname, printer, hasOptions, optionsObj);
    worker->Queue();
    return env.Undefined();
  }));

  // printDirect AsyncWorker with enhanced format validation and options support
  exports.Set("printDirect", Function::New(env, [](const CallbackInfo& info)->Value{
    Env env = info.Env();
    if (info.Length() < 4) {
      TypeError::New(env, "printDirect requires at least 4 arguments: data, printer, docname, type").ThrowAsJavaScriptException();
      return env.Null();
    }

    // data arg: string or Buffer
    bool isBuffer = info[0].IsBuffer();
    bool isString = info[0].IsString();
    if (!isBuffer && !isString) {
      TypeError::New(env, "Argument 0 must be a string or Buffer").ThrowAsJavaScriptException();
      return env.Null();
    }

    std::string printer = info[1].IsString() ? info[1].As<String>().Utf8Value() : std::string();
    std::string docname = info[2].IsString() ? info[2].As<String>().Utf8Value() : std::string();
    std::string type = info[3].IsString() ? info[3].As<String>().Utf8Value() : std::string();

    // Enhanced format validation
    std::string cupsFormat;
    const auto &formatMap = getPrinterFormatMap();
    auto it = formatMap.find(type);
    if (it != formatMap.end()) {
      cupsFormat = it->second;
    } else {
      // If not found in map, try using the type directly (for backward compatibility)
      cupsFormat = type;
    }

    // Enhanced options handling
    bool hasOptions = info.Length() > 4 && info[4].IsObject() && !info[4].IsFunction();
    
    // callback may be at position 5 or 6 depending on options presence
    Function cb = Function::New(env, [](const CallbackInfo&){ });
    if (info.Length() > 5 && info[5].IsFunction()) cb = info[5].As<Function>();
    else if (info.Length() > 4 && info[4].IsFunction()) cb = info[4].As<Function>();

    // Prepare data: for buffers, get pointer and length
    std::string smallData;
    std::string tmpFilename;
    bool useTempFile = false;
    size_t dataLen = 0;
    if (isBuffer) {
      Buffer<char> buf = info[0].As<Buffer<char>>();
      dataLen = buf.Length();
      if (dataLen > STREAM_THRESHOLD) {
        // create temp file and write synchronously (initial compromise)
        char tpl[] = "/tmp/node_printerXXXXXX";
        int fd = mkstemp(tpl);
        if (fd == -1) {
          Error::New(env, "Unable to create temporary file").ThrowAsJavaScriptException();
          return env.Null();
        }
        ssize_t written = write(fd, buf.Data(), dataLen);
        close(fd);
        if (written != (ssize_t)dataLen) {
          // attempt to unlink
          unlink(tpl);
          Error::New(env, "Unable to write full buffer to temp file").ThrowAsJavaScriptException();
          return env.Null();
        }
        tmpFilename = std::string(tpl);
        useTempFile = true;
      } else {
        smallData.assign(buf.Data(), buf.Length());
      }
    } else {
      // string
      std::string s = info[0].As<String>().Utf8Value();
      dataLen = s.size();
      if (dataLen > STREAM_THRESHOLD) {
        char tpl[] = "/tmp/node_printerXXXXXX";
        int fd = mkstemp(tpl);
        if (fd == -1) {
          Error::New(env, "Unable to create temporary file").ThrowAsJavaScriptException();
          return env.Null();
        }
        ssize_t written = write(fd, s.data(), dataLen);
        close(fd);
        if (written != (ssize_t)dataLen) {
          unlink(tpl);
          Error::New(env, "Unable to write full string to temp file").ThrowAsJavaScriptException();
          return env.Null();
        }
        tmpFilename = std::string(tpl);
        useTempFile = true;
      } else {
        smallData = s;
      }
    }

    class PrintDirectWorker : public AsyncWorker {
    public:
      PrintDirectWorker(Function& cb, bool useTmp, std::string tmpF, std::string data, 
                       std::string pr, std::string dn, std::string tp, bool useOpts, Object opts)
        : AsyncWorker(cb), useTemp(useTmp), tmpFilename(std::move(tmpF)), dataBuf(std::move(data)), 
          printer(std::move(pr)), docname(std::move(dn)), type(std::move(tp)), 
          useOptions(useOpts), jobId(0) {
        if (useOptions) {
          // Initialize options manager - if it fails, useOptions will remain true but optionsManager will be default
          optionsManager = CupsOptionsManager(opts);
        }
      }
      
      void Execute() override {
        if (useTemp) {
          // use cupsPrintFile on temp file with options support
          int id;
          if (useOptions) {
            id = cupsPrintFile(printer.c_str(), tmpFilename.c_str(), docname.c_str(), 
                             optionsManager.getNumOptions(), optionsManager.get());
          } else {
            id = cupsPrintFile(printer.c_str(), tmpFilename.c_str(), docname.c_str(), 0, NULL);
          }
          
          if (id == 0) {
            std::string err = cupsLastErrorString();
            if (err.empty()) err = "Unknown printing error";
            SetError(err);
            unlink(tmpFilename.c_str());
            return;
          }
          jobId = id;
          unlink(tmpFilename.c_str());
          return;
        }
        
        // stream from memory: create job and write with options support
        int job_id;
        if (useOptions) {
          job_id = cupsCreateJob(CUPS_HTTP_DEFAULT, printer.c_str(), docname.c_str(), 
                               optionsManager.getNumOptions(), optionsManager.get());
        } else {
          job_id = cupsCreateJob(CUPS_HTTP_DEFAULT, printer.c_str(), docname.c_str(), 0, NULL);
        }
        
        if (job_id == 0) {
          std::string err = cupsLastErrorString();
          if (err.empty()) err = "Failed to create print job";
          SetError(err);
          return;
        }
        
        if (HTTP_CONTINUE != cupsStartDocument(CUPS_HTTP_DEFAULT, printer.c_str(), job_id, 
                                              docname.c_str(), type.c_str(), 1)) {
          std::string err = cupsLastErrorString();
          if (err.empty()) err = "Failed to start document";
          SetError(err);
          return;
        }
        
        if (HTTP_CONTINUE != cupsWriteRequestData(CUPS_HTTP_DEFAULT, dataBuf.c_str(), dataBuf.size())) {
          cupsFinishDocument(CUPS_HTTP_DEFAULT, printer.c_str());
          std::string err = cupsLastErrorString();
          if (err.empty()) err = "Failed to write document data";
          SetError(err);
          return;
        }
        
        cupsFinishDocument(CUPS_HTTP_DEFAULT, printer.c_str());
        jobId = job_id;
      }
      
      void OnOK() override {
        HandleScope scope(Env());
        Callback().Call({ Env().Null(), Number::New(Env(), jobId) });
      }
      
    private:
      bool useTemp;
      std::string tmpFilename;
      std::string dataBuf;
      std::string printer, docname, type;
      bool useOptions;
      CupsOptionsManager optionsManager;
      int jobId;
    };

    Object optionsObj = hasOptions ? info[4].As<Object>() : Object::New(env);
    Function finalCb = cb;
    PrintDirectWorker* worker = new PrintDirectWorker(finalCb, useTempFile, tmpFilename, smallData, 
                                                     printer, docname, cupsFormat, hasOptions, optionsObj);
    worker->Queue();
    return env.Undefined();
  }));
    // getDefaultPrinterName: intentionally returns undefined (see notes in TODO)
    exports.Set("getDefaultPrinterName", Function::New(env, [](const CallbackInfo& info)->Value{
      Env env = info.Env();
      return env.Undefined();
    }));

    // getSupportedJobCommands: currently only CANCEL is supported
    exports.Set("getSupportedJobCommands", Function::New(env, [](const CallbackInfo& info)->Value{
      Env env = info.Env();
      Array result = Array::New(env, 1);
      result.Set((uint32_t)0, String::New(env, "CANCEL"));
      return result;
    }));

    // getSupportedPrintFormats: return keys from getPrinterFormatMap
    exports.Set("getSupportedPrintFormats", Function::New(env, [](const CallbackInfo& info)->Value{
      Env env = info.Env();
      const auto &fm = getPrinterFormatMap();
      Array result = Array::New(env, fm.size());
      uint32_t i = 0;
      for (const auto &kv : fm) {
        result.Set(i++, String::New(env, kv.first));
      }
      return result;
    }));

    // setJob: enhanced CANCEL command support with better error handling
    exports.Set("setJob", Function::New(env, [](const CallbackInfo& info)->Value{
      Env env = info.Env();
      if (info.Length() < 3 || !info[0].IsString() || !info[1].IsNumber() || !info[2].IsString()) {
        TypeError::New(env, "printer name, job id and command required").ThrowAsJavaScriptException();
        return env.Null();
      }
      std::string printer = info[0].As<String>().Utf8Value();
      int jobId = info[1].As<Number>().Int32Value();
      std::string cmd = info[2].As<String>().Utf8Value();
      
      if (jobId < 0) {
        Error::New(env, "Invalid job ID").ThrowAsJavaScriptException();
        return env.Null();
      }
      
      if (cmd == "CANCEL") {
        int result = cupsCancelJob(printer.c_str(), jobId);
        if (result != 1) {
          std::string error = std::string("Failed to cancel job: ") + cupsLastErrorString();
          Error::New(env, error).ThrowAsJavaScriptException();
          return env.Null();
        }
        return Boolean::New(env, true);
      }
      
      Error::New(env, "wrong job command. use getSupportedJobCommands to see the possible commands").ThrowAsJavaScriptException();
      return env.Null();
    }));
  // Other POSIX functions will be implemented later
  return exports;
}

NODE_API_MODULE(node_printer, Init)
