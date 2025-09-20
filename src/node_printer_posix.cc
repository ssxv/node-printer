#include <napi.h>
#include <cups/cups.h>
#include <cups/ppd.h>

#include <string>
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
  m["PRINTING"] = IPP_JOB_PROCESSING;
  m["PRINTED"] = IPP_JOB_COMPLETED;
  m["PAUSED"] = IPP_JOB_HELD;
  m["PENDING"] = IPP_JOB_PENDING;
  m["PAUSED"] = IPP_JOB_STOPPED;
  m["CANCELLED"] = IPP_JOB_CANCELLED;
  m["ABORTED"] = IPP_JOB_ABORTED;
  return m;
}

static std::map<std::string, std::string> getPrinterFormatMap() {
  static std::map<std::string, std::string> m;
  if (!m.empty()) return m;
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
  return m;
}

// Threshold (bytes) where we switch to temp-file fallback
static const size_t STREAM_THRESHOLD = 4 * 1024 * 1024; // 4 MiB

// Convert cups_job_t -> JS object
static void parseJobObject(const cups_job_t* job, Env env, Object& out) {
  out.Set("id", Number::New(env, job->id));
  out.Set("name", String::New(env, job->title?job->title:""));
  out.Set("printerName", String::New(env, job->dest?job->dest:""));
  out.Set("user", String::New(env, job->user?job->user:""));

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

  Array statusArr = Array::New(env);
  uint32_t si = 0;
  for (const auto &kv : getJobStatusMap()) {
    if (job->state == kv.second) {
      statusArr.Set(si++, String::New(env, kv.first));
      break;
    }
  }
  if (si == 0) {
    // unknown status -> include raw numeric
    statusArr.Set(si++, String::New(env, "UNKNOWN"));
  }
  out.Set("status", statusArr);

  // convert seconds -> ms for JS Date
  double creationMs = static_cast<double>(job->creation_time) * 1000.0;
  double completedMs = static_cast<double>(job->completed_time) * 1000.0;
  double processingMs = static_cast<double>(job->processing_time) * 1000.0;
  out.Set("creationTime", Date::New(env, creationMs));
  out.Set("completedTime", Date::New(env, completedMs));
  out.Set("processingTime", Date::New(env, processingMs));
}

// Parse printer info and include options and active jobs
static std::string parsePrinterInfo(const cups_dest_t* printer, Env env, Object& out) {
  out.Set("name", String::New(env, printer->name ? printer->name : ""));
  out.Set("isDefault", Boolean::New(env, printer->is_default));
  if (printer->instance) out.Set("instance", String::New(env, printer->instance));

  Object optionsObj = Object::New(env);
  for (int i = 0; i < printer->num_options; ++i) {
    cups_option_t *opt = &printer->options[i];
    optionsObj.Set(String::New(env, opt->name), String::New(env, opt->value));
  }
  out.Set("options", optionsObj);

  // Get printer jobs
  cups_job_t *jobs = nullptr;
  int totalJobs = cupsGetJobs(&jobs, printer->name, 0, CUPS_WHICHJOBS_ACTIVE);
  if (totalJobs > 0) {
    Array jobsArr = Array::New(env, totalJobs);
    for (int i = 0; i < totalJobs; ++i) {
      Object jobObj = Object::New(env);
      parseJobObject(&jobs[i], env, jobObj);
      jobsArr.Set((uint32_t)i, jobObj);
    }
    out.Set("jobs", jobsArr);
  }
  cupsFreeJobs(totalJobs, jobs);
  return std::string();
}

// getPrinters implementation
static Value GetPrinters(const CallbackInfo& info) {
  Env env = info.Env();
  cups_dest_t *printers = nullptr;
  int printers_size = cupsGetDests(&printers);
  Array result = Array::New(env, printers_size);
  for (int i = 0; i < printers_size; ++i) {
    Object p = Object::New(env);
    std::string err = parsePrinterInfo(&printers[i], env, p);
    if (!err.empty()) {
      Error::New(env, err).ThrowAsJavaScriptException();
      cupsFreeDests(printers_size, printers);
      return env.Null();
    }
    result.Set((uint32_t)i, p);
  }
  cupsFreeDests(printers_size, printers);
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
    cups_dest_t *p = cupsGetDest(name.c_str(), NULL, n, printers);
    if (!p) {
      cupsFreeDests(n, printers);
      Error::New(env, "Printer not found").ThrowAsJavaScriptException();
      return env.Null();
    }
    Object out = Object::New(env);
    parsePrinterInfo(p, env, out);
    cupsFreeDests(n, printers);
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
    cups_job_t *jobs = nullptr;
    int totalJobs = cupsGetJobs(&jobs, name.c_str(), 0, CUPS_WHICHJOBS_ALL);
    cups_job_t *found = nullptr;
    Object out = Object::New(env);
    if (totalJobs > 0) {
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
    cups_dest_t *p = cupsGetDest(name.c_str(), NULL, n, printers);
    if (!p) {
      cupsFreeDests(n, printers);
      Error::New(env, "Printer not found").ThrowAsJavaScriptException();
      return env.Null();
    }

    // Build a conservative, non-deprecated representation of driver options
    // using the cups destination options available via cupsGetDests.
    // This intentionally avoids the deprecated ppd_* helper APIs. A future
    // enhancement should call cupsCopyDestInfo/cupsCopyDest* to obtain full
    // driver/PPD option metadata (choices, marked flags, groups).
    Object driverOptions = Object::New(env);
    for (int i = 0; i < p->num_options; ++i) {
      cups_option_t *opt = &p->options[i];
      // Represent each option as its current string value. This is a
      // simplified view compared to the detailed PPD choice map previously
      // produced, but it is non-deprecated and portable across CUPS versions.
      driverOptions.Set(String::New(env, opt->name), String::New(env, opt->value));
    }

    cupsFreeDests(n, printers);
    return driverOptions;
  }));
  // printFile AsyncWorker
  exports.Set("printFile", Function::New(env, [](const CallbackInfo& info)->Value{
    Env env = info.Env();
    if (info.Length() < 3 || !info[0].IsString() || !info[1].IsString() || !info[2].IsString()) {
      TypeError::New(env, "filename, docname and printer required").ThrowAsJavaScriptException();
      return env.Null();
    }
    std::string filename = info[0].As<String>().Utf8Value();
    std::string docname = info[1].As<String>().Utf8Value();
    std::string printer = info[2].As<String>().Utf8Value();

    // Options are currently ignored or optional

    // Create an AsyncWorker
    class PrintFileWorker : public AsyncWorker {
    public:
      PrintFileWorker(Function& cb, std::string fn, std::string dn, std::string pr)
        : AsyncWorker(cb), filename(std::move(fn)), docname(std::move(dn)), printer(std::move(pr)), jobId(0) {}
      void Execute() override {
        int id = cupsPrintFile(printer.c_str(), filename.c_str(), docname.c_str(), 0, NULL);
        if (id == 0) {
          std::string err = cupsLastErrorString();
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
      int jobId;
    };

    Function cb = info.Length() > 3 && info[3].IsFunction() ? info[3].As<Function>() : Function::New(env, [](const CallbackInfo&){ });
    PrintFileWorker* worker = new PrintFileWorker(cb, filename, docname, printer);
    worker->Queue();
    return env.Undefined();
  }));

  // printDirect AsyncWorker with Option B (temp-file fallback)
  exports.Set("printDirect", Function::New(env, [](const CallbackInfo& info)->Value{
    Env env = info.Env();
    if (info.Length() < 5) {
      TypeError::New(env, "expected arguments: data, printerName, docName, type, options, callback?").ThrowAsJavaScriptException();
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

    // options parameter may be object; ignore for now

    // callback may be at position 5 or 6 depending on options presence; for simplicity accept callback as last arg if function
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
      PrintDirectWorker(Function& cb, bool useTmp, std::string tmpF, std::string data, std::string pr, std::string dn, std::string tp)
        : AsyncWorker(cb), useTemp(useTmp), tmpFilename(std::move(tmpF)), dataBuf(std::move(data)), printer(std::move(pr)), docname(std::move(dn)), type(std::move(tp)), jobId(0) {}
      void Execute() override {
        if (useTemp) {
          // use cupsPrintFile on temp file
          int id = cupsPrintFile(printer.c_str(), tmpFilename.c_str(), docname.c_str(), 0, NULL);
          if (id == 0) {
            std::string err = cupsLastErrorString();
            SetError(err);
            // attempt cleanup
            unlink(tmpFilename.c_str());
            return;
          }
          jobId = id;
          // cleanup
          unlink(tmpFilename.c_str());
          return;
        }
        // stream from memory: create job and write
        int job_id = cupsCreateJob(CUPS_HTTP_DEFAULT, printer.c_str(), docname.c_str(), 0, NULL);
        if (job_id == 0) {
          SetError(cupsLastErrorString());
          return;
        }
        if (HTTP_CONTINUE != cupsStartDocument(CUPS_HTTP_DEFAULT, printer.c_str(), job_id, docname.c_str(), type.c_str(), 1)) {
          SetError(cupsLastErrorString());
          return;
        }
        if (HTTP_CONTINUE != cupsWriteRequestData(CUPS_HTTP_DEFAULT, dataBuf.c_str(), dataBuf.size())) {
          cupsFinishDocument(CUPS_HTTP_DEFAULT, printer.c_str());
          SetError(cupsLastErrorString());
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
      int jobId;
    };

    Function finalCb = cb;
    PrintDirectWorker* worker = new PrintDirectWorker(finalCb, useTempFile, tmpFilename, smallData, printer, docname, type);
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

    // setJob: support CANCEL command via cupsCancelJob
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
        Error::New(env, "Wrong job number").ThrowAsJavaScriptException();
        return env.Null();
      }
      if (cmd == "CANCEL") {
        int ok = cupsCancelJob(printer.c_str(), jobId);
        return Boolean::New(env, ok == 1);
      }
      Error::New(env, "wrong job command. use getSupportedJobCommands to see the possible commands").ThrowAsJavaScriptException();
      return env.Null();
    }));
  // Other POSIX functions will be implemented later
  return exports;
}

NODE_API_MODULE(node_printer, Init)
