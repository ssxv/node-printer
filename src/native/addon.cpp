// Main N-API addon entry point
// Coordinates platform-specific implementations and exposes unified API

#include <napi.h>
#include "printer_api.h"
#include "job_api.h"
#include "errors.h"
#include <memory>

#ifdef _WIN32
#include "win/printers_win.cpp"
#include "win/jobs_win.cpp"
#else
#include "cups/printers_cups.cpp"
#include "cups/jobs_cups.cpp"
#endif

namespace NodePrinter {

// Factory implementations
std::unique_ptr<IPrinterAPI> createPrinterAPI() {
#ifdef _WIN32
    return std::make_unique<WinPrinterAPI>();
#else
    return std::make_unique<CupsPrinterAPI>();
#endif
}

std::unique_ptr<IJobAPI> createJobAPI() {
#ifdef _WIN32
    return std::make_unique<WinJobAPI>();
#else
    return std::make_unique<CupsJobAPI>();
#endif
}

// Global API instances (initialized at module load)
static std::unique_ptr<IPrinterAPI> g_printerAPI;
static std::unique_ptr<IJobAPI> g_jobAPI;

/**
 * Convert PrinterException to enhanced Napi::Error
 */
Napi::Error createEnhancedNapiError(Napi::Env env, const PrinterException& e) {
    Napi::Error napiError = Napi::Error::New(env, e.what());
    
    // Add error code
    napiError.Set("code", Napi::String::New(env, printerErrorCodeToString(e.getCode())));
    
    // Add platform-specific error code if available
    if (e.getPlatformCode() != 0) {
        napiError.Set("platformCode", Napi::Number::New(env, e.getPlatformCode()));
    }
    
    // Add error type
    napiError.Set("type", Napi::String::New(env, "PrinterError"));
    
    return napiError;
}

/**
 * Handle exceptions with enhanced error reporting
 */
void handlePrinterException(Napi::Env env, const PrinterException& e) {
    createEnhancedNapiError(env, e).ThrowAsJavaScriptException();
}

void handleException(Napi::Env env, const std::exception& e) {
    Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
}

/**
 * Convert PrinterInfo to JavaScript object
 */
Napi::Object printerInfoToJS(const PrinterInfo& info, Napi::Env env) {
    Napi::Object obj = Napi::Object::New(env);
    obj.Set("name", info.name);
    obj.Set("isDefault", info.isDefault);
    
    // Status as array for compatibility with legacy format
    Napi::Array statusArr = Napi::Array::New(env, 1);
    statusArr.Set(0u, info.state);
    obj.Set("status", statusArr);
    
    if (!info.location.empty()) {
        obj.Set("location", info.location);
    }
    
    if (!info.description.empty()) {
        obj.Set("comment", info.description); // Use 'comment' for backward compatibility
    }
    
    return obj;
}

/**
 * Convert JobInfo to JavaScript object
 */
Napi::Object jobInfoToJS(const JobInfo& info, Napi::Env env) {
    Napi::Object obj = Napi::Object::New(env);
    obj.Set("id", info.id);
    
    // Status as array for compatibility
    Napi::Array statusArr = Napi::Array::New(env, 1);
    statusArr.Set(0u, info.state);
    obj.Set("status", statusArr);
    
    if (!info.printer.empty()) {
        obj.Set("printerName", info.printer);
    }
    
    if (!info.title.empty()) {
        obj.Set("name", info.title); // Use 'name' for job title in legacy API
    }
    
    if (!info.user.empty()) {
        obj.Set("user", info.user);
    }
    
    if (info.creationTime > 0) {
        obj.Set("creationTime", static_cast<double>(info.creationTime));
    }
    
    if (info.processingTime > 0) {
        obj.Set("processingTime", static_cast<double>(info.processingTime));
    }
    
    if (info.completedTime > 0) {
        obj.Set("completedTime", static_cast<double>(info.completedTime));
    }
    
    if (info.pages > 0) {
        obj.Set("totalPages", info.pages);
    }
    
    if (info.size > 0) {
        obj.Set("size", static_cast<double>(info.size));
    }
    
    return obj;
}

/**
 * Convert JavaScript print options to PrintOptions struct
 */
PrintOptions jsTorintOptions(const Napi::Value& jsOptions) {
    PrintOptions options;
    
    if (jsOptions.IsObject()) {
        Napi::Object optObj = jsOptions.As<Napi::Object>();
        
        if (optObj.Has("copies") && optObj.Get("copies").IsNumber()) {
            options.copies = optObj.Get("copies").As<Napi::Number>().Int32Value();
        }
        
        if (optObj.Has("duplex") && optObj.Get("duplex").IsBoolean()) {
            options.duplex = optObj.Get("duplex").As<Napi::Boolean>().Value();
        }
        
        if (optObj.Has("color") && optObj.Get("color").IsBoolean()) {
            options.color = optObj.Get("color").As<Napi::Boolean>().Value();
        }
        
        if (optObj.Has("paperSize") && optObj.Get("paperSize").IsString()) {
            options.paperSize = optObj.Get("paperSize").As<Napi::String>().Utf8Value();
        }
        
        if (optObj.Has("orientation") && optObj.Get("orientation").IsString()) {
            options.orientation = optObj.Get("orientation").As<Napi::String>().Utf8Value();
        }
        
        if (optObj.Has("docname") && optObj.Get("docname").IsString()) {
            options.jobName = optObj.Get("docname").As<Napi::String>().Utf8Value();
        } else if (optObj.Has("jobName") && optObj.Get("jobName").IsString()) {
            options.jobName = optObj.Get("jobName").As<Napi::String>().Utf8Value();
        }
    }
    
    return options;
}

// Async worker classes for non-blocking operations
class GetPrintersWorker : public Napi::AsyncWorker {
public:
    GetPrintersWorker(Napi::Function& callback) 
        : Napi::AsyncWorker(callback) {}
    
    void Execute() override {
        try {
            printers = g_printerAPI->getPrinters();
        } catch (const std::exception& e) {
            SetError(e.what());
        }
    }
    
    void OnOK() override {
        Napi::Env env = Env();
        Napi::Array result = Napi::Array::New(env, printers.size());
        
        for (size_t i = 0; i < printers.size(); ++i) {
            result[i] = printerInfoToJS(printers[i], env);
        }
        
        Callback().Call({env.Null(), result});
    }

private:
    std::vector<PrinterInfo> printers;
};

// N-API function bindings

Napi::Value GetPrinters(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() > 0 && info[0].IsFunction()) {
        // Callback mode (async)
        Napi::Function callback = info[0].As<Napi::Function>();
        GetPrintersWorker* worker = new GetPrintersWorker(callback);
        worker->Queue();
        return env.Undefined();
    }
    
    // Synchronous mode
    try {
        std::vector<PrinterInfo> printers = g_printerAPI->getPrinters();
        Napi::Array result = Napi::Array::New(env, printers.size());
        
        for (size_t i = 0; i < printers.size(); ++i) {
            result[i] = printerInfoToJS(printers[i], env);
        }
        
        return result;
    } catch (const std::exception& e) {
        handleException(env, e);
        return env.Null();
    }
}

Napi::Value GetDefaultPrinterName(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    try {
        std::string defaultName = g_printerAPI->getDefaultPrinterName();
        return Napi::String::New(env, defaultName);
    } catch (const std::exception& e) {
        handleException(env, e);
        return env.Null();
    }
}

Napi::Value GetSupportedPrintFormats(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    try {
        auto formats = g_printerAPI->getSupportedFormats();
        Napi::Array result = Napi::Array::New(env, formats.size());
        
        for (size_t i = 0; i < formats.size(); ++i) {
            result[i] = Napi::String::New(env, formats[i]);
        }
        
        return result;
    } catch (const std::exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
        return env.Null();
    }
}

Napi::Value GetPrinter(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "Printer name required").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    try {
        std::string name = info[0].As<Napi::String>().Utf8Value();
        PrinterInfo printer = g_printerAPI->getPrinter(name);
        return printerInfoToJS(printer, env);
    } catch (const std::exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
        return env.Null();
    }
}

Napi::Value GetPrinterDriverOptions(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "Printer name required").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    try {
        std::string name = info[0].As<Napi::String>().Utf8Value();
        return g_printerAPI->getDriverOptions(name, env);
    } catch (const std::exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
        return env.Null();
    }
}

Napi::Value PrintFile(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 2) {
        Napi::TypeError::New(env, "Missing arguments: filename and printer required").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    try {
        PrintFileRequest request;
        
        // Extract filename and printer from separate arguments
        request.filename = info[0].As<Napi::String>().Utf8Value();
        request.printer = info[1].As<Napi::String>().Utf8Value();
        
        // Extract options from third argument if present
        if (info.Length() > 2 && info[2].IsObject()) {
            request.options = jsTorintOptions(info[2]);
        }
        
        int jobId = g_jobAPI->printFile(request);
        return Napi::Number::New(env, jobId);
    } catch (const std::exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
        return env.Null();
    }
}

Napi::Value PrintDirect(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 2) {
        Napi::TypeError::New(env, "Missing arguments: data and printer required").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    try {
        PrintRawRequest request;
        
        // Extract data from first argument
        if (info[0].IsBuffer()) {
            Napi::Buffer<uint8_t> buffer = info[0].As<Napi::Buffer<uint8_t>>();
            request.data.assign(buffer.Data(), buffer.Data() + buffer.Length());
        } else {
            Napi::TypeError::New(env, "Data must be a Buffer").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        // Extract printer from second argument
        request.printer = info[1].As<Napi::String>().Utf8Value();
        
        // Extract format/type from third argument if present
        if (info.Length() > 2 && info[2].IsString()) {
            request.format = info[2].As<Napi::String>().Utf8Value();
        }
        
        // Extract options from fourth argument if present
        if (info.Length() > 3 && info[3].IsObject()) {
            request.options = jsTorintOptions(info[3]);
        }
        
        int jobId = g_jobAPI->printRaw(request);
        return Napi::Number::New(env, jobId);
    } catch (const std::exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
        return env.Null();
    }
}

Napi::Value GetJob(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 2) {
        Napi::TypeError::New(env, "Missing arguments: printer and jobId required").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    try {
        std::string printer = info[0].As<Napi::String>().Utf8Value();
        int jobId = info[1].As<Napi::Number>().Int32Value();
        
        JobInfo job = g_jobAPI->getJob(printer, jobId);
        return jobInfoToJS(job, env);
    } catch (const std::exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
        return env.Null();
    }
}

Napi::Value GetJobs(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    try {
        std::string printer = "";
        if (info.Length() > 0 && info[0].IsString()) {
            printer = info[0].As<Napi::String>().Utf8Value();
        }
        
        std::vector<JobInfo> jobs = g_jobAPI->getJobs(printer);
        Napi::Array result = Napi::Array::New(env, jobs.size());
        
        for (size_t i = 0; i < jobs.size(); ++i) {
            result[i] = jobInfoToJS(jobs[i], env);
        }
        
        return result;
    } catch (const std::exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
        return env.Null();
    }
}

Napi::Value SetJob(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 3) {
        Napi::TypeError::New(env, "Missing arguments: printer, jobId, and command required").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    try {
        std::string printer = info[0].As<Napi::String>().Utf8Value();
        int jobId = info[1].As<Napi::Number>().Int32Value();
        std::string cmdStr = info[2].As<Napi::String>().Utf8Value();
        
        JobCommand command;
        if (cmdStr == "pause") command = JobCommand::PAUSE;
        else if (cmdStr == "resume") command = JobCommand::RESUME;
        else if (cmdStr == "cancel") command = JobCommand::CANCEL;
        else {
            Napi::TypeError::New(env, "Invalid command. Use 'pause', 'resume', or 'cancel'").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        g_jobAPI->setJob(printer, jobId, command);
        return env.Undefined();
    } catch (const std::exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
        return env.Null();
    }
}

// Export initialization
Napi::Object Init(Napi::Env env, Napi::Object exports) {
    // Initialize platform-specific APIs
    try {
        g_printerAPI = createPrinterAPI();
        g_jobAPI = createJobAPI();
    } catch (const std::exception& e) {
        Napi::Error::New(env, std::string("Failed to initialize printer APIs: ") + e.what()).ThrowAsJavaScriptException();
        return exports;
    }
    
    // Export functions - Complete Native Abstraction Layer
    exports.Set("getPrinters", Napi::Function::New(env, GetPrinters));
    exports.Set("getPrinter", Napi::Function::New(env, GetPrinter));
    exports.Set("getDefaultPrinterName", Napi::Function::New(env, GetDefaultPrinterName));
    exports.Set("getSupportedPrintFormats", Napi::Function::New(env, GetSupportedPrintFormats));
    exports.Set("getPrinterDriverOptions", Napi::Function::New(env, GetPrinterDriverOptions));
    exports.Set("printDirect", Napi::Function::New(env, PrintDirect));
    exports.Set("printFile", Napi::Function::New(env, PrintFile));
    exports.Set("getJob", Napi::Function::New(env, GetJob));
    exports.Set("getJobs", Napi::Function::New(env, GetJobs));
    exports.Set("setJob", Napi::Function::New(env, SetJob));
    
    return exports;
}

NODE_API_MODULE(node_printer, Init)

} // namespace NodePrinter