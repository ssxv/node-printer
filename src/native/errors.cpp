#include "errors.h"
#include <sstream>
#include <algorithm>

#ifdef _WIN32
    #include <windows.h>
    #include <winspool.h>
#else
    #include <cups/cups.h>
#endif

namespace NodePrinter {

std::string printerErrorCodeToString(PrinterErrorCode code) {
    switch (code) {
        case PrinterErrorCode::PRINTER_NOT_FOUND: return "PRINTER_NOT_FOUND";
        case PrinterErrorCode::PRINTER_OFFLINE: return "PRINTER_OFFLINE";
        case PrinterErrorCode::ACCESS_DENIED: return "ACCESS_DENIED";
        case PrinterErrorCode::JOB_NOT_FOUND: return "JOB_NOT_FOUND";
        case PrinterErrorCode::DRIVER_ERROR: return "DRIVER_ERROR";
        case PrinterErrorCode::INVALID_ARGUMENTS: return "INVALID_ARGUMENTS";
        case PrinterErrorCode::FILE_NOT_FOUND: return "FILE_NOT_FOUND";
        case PrinterErrorCode::UNSUPPORTED_FORMAT: return "UNSUPPORTED_FORMAT";
        case PrinterErrorCode::UNKNOWN: return "UNKNOWN";
        default: return "UNKNOWN";
    }
}

PrinterException::PrinterException(
    const std::string& message, 
    PrinterErrorCode code,
    int platformCode
) : message_(message), code_(code), platformCode_(platformCode) {
}

const char* PrinterException::what() const noexcept {
    return message_.c_str();
}

PrinterErrorCode PrinterException::getCode() const noexcept {
    return code_;
}

int PrinterException::getPlatformCode() const noexcept {
    return platformCode_;
}

std::string PrinterException::getFullMessage() const {
    std::ostringstream ss;
    ss << message_ << " [" << printerErrorCodeToString(code_);
    if (platformCode_ != 0) {
        ss << ", platform code: " << platformCode_;
    }
    ss << "]";
    return ss.str();
}

namespace ErrorMappers {

#ifdef _WIN32
PrinterErrorCode mapWindowsError(unsigned long dwError, const std::string& context) {
    switch (dwError) {
        case ERROR_INVALID_PRINTER_NAME:
        case ERROR_INVALID_HANDLE:
            return PrinterErrorCode::PRINTER_NOT_FOUND;
        
        case ERROR_PRINTER_NOT_FOUND:
            return PrinterErrorCode::PRINTER_NOT_FOUND;
        
        case ERROR_ACCESS_DENIED:
        case ERROR_PRIVILEGE_NOT_HELD:
            return PrinterErrorCode::ACCESS_DENIED;
        
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
            if (context.find("file") != std::string::npos) {
                return PrinterErrorCode::FILE_NOT_FOUND;
            }
            return PrinterErrorCode::UNKNOWN;
        
        case ERROR_PRINTER_DRIVER_IN_USE:  
        case ERROR_SPOOL_FILE_NOT_FOUND:
            return PrinterErrorCode::PRINTER_OFFLINE;
        
        case ERROR_INVALID_PARAMETER:
        case ERROR_INVALID_DATA:
            return PrinterErrorCode::INVALID_ARGUMENTS;
        
        case ERROR_UNKNOWN_PRINT_MONITOR:
        case ERROR_PRINTER_DRIVER_ALREADY_INSTALLED:
            return PrinterErrorCode::DRIVER_ERROR;
        
        case 0: // No error
            return PrinterErrorCode::UNKNOWN;
        
        default:
            return PrinterErrorCode::UNKNOWN;
    }
}

PrinterException createWindowsError(const std::string& message, unsigned long dwError) {
    if (dwError == 0) {
        dwError = GetLastError();
    }
    
    PrinterErrorCode code = mapWindowsError(dwError, message);
    
    // Get Windows error message
    std::string fullMessage = message;
    if (dwError != 0) {
        LPWSTR lpMsgBuf = nullptr;
        DWORD bufLen = FormatMessageW(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | 
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr, dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPWSTR)&lpMsgBuf, 0, nullptr);
        
        if (bufLen && lpMsgBuf) {
            // Convert to UTF-8 (simplified conversion)
            int utf8Len = WideCharToMultiByte(CP_UTF8, 0, lpMsgBuf, -1, nullptr, 0, nullptr, nullptr);
            if (utf8Len > 0) {
                std::string errorStr(utf8Len - 1, '\0');
                WideCharToMultiByte(CP_UTF8, 0, lpMsgBuf, -1, &errorStr[0], utf8Len, nullptr, nullptr);
                
                // Remove trailing whitespace
                size_t end = errorStr.find_last_not_of(" \t\r\n");
                if (end != std::string::npos) {
                    errorStr = errorStr.substr(0, end + 1);
                }
                
                fullMessage += ": " + errorStr;
            }
            LocalFree(lpMsgBuf);
        }
    }
    
    return PrinterException(fullMessage, code, static_cast<int>(dwError));
}

#else // POSIX/CUPS

PrinterErrorCode mapCupsError(const std::string& cupsError) {
    if (cupsError.empty()) {
        return PrinterErrorCode::UNKNOWN;
    }
    
    std::string lowerError = cupsError;
    std::transform(lowerError.begin(), lowerError.end(), lowerError.begin(), ::tolower);
    
    if (lowerError.find("not found") != std::string::npos ||
        lowerError.find("no such") != std::string::npos) {
        if (lowerError.find("printer") != std::string::npos ||
            lowerError.find("destination") != std::string::npos) {
            return PrinterErrorCode::PRINTER_NOT_FOUND;
        }
        if (lowerError.find("job") != std::string::npos) {
            return PrinterErrorCode::JOB_NOT_FOUND;
        }
        if (lowerError.find("file") != std::string::npos) {
            return PrinterErrorCode::FILE_NOT_FOUND;
        }
    }
    
    if (lowerError.find("offline") != std::string::npos ||
        lowerError.find("unavailable") != std::string::npos ||
        lowerError.find("stopped") != std::string::npos) {
        return PrinterErrorCode::PRINTER_OFFLINE;
    }
    
    if (lowerError.find("permission") != std::string::npos ||
        lowerError.find("access denied") != std::string::npos ||
        lowerError.find("unauthorized") != std::string::npos) {
        return PrinterErrorCode::ACCESS_DENIED;
    }
    
    if (lowerError.find("driver") != std::string::npos ||
        lowerError.find("ppd") != std::string::npos) {
        return PrinterErrorCode::DRIVER_ERROR;
    }
    
    if (lowerError.find("invalid") != std::string::npos ||
        lowerError.find("bad") != std::string::npos ||
        lowerError.find("malformed") != std::string::npos) {
        return PrinterErrorCode::INVALID_ARGUMENTS;
    }
    
    if (lowerError.find("format") != std::string::npos ||
        lowerError.find("unsupported") != std::string::npos) {
        return PrinterErrorCode::UNSUPPORTED_FORMAT;
    }
    
    return PrinterErrorCode::UNKNOWN;
}

PrinterException createCupsError(const std::string& message, const std::string& cupsError) {
    std::string errorString = cupsError.empty() ? cupsLastErrorString() : cupsError;
    PrinterErrorCode code = mapCupsError(errorString);
    
    std::string fullMessage = message;
    if (!errorString.empty()) {
        fullMessage += ": " + errorString;
    }
    
    return PrinterException(fullMessage, code, cupsLastError());
}

#endif

PrinterErrorCode mapGenericError(const std::string& message) {
    std::string lowerMessage = message;
    std::transform(lowerMessage.begin(), lowerMessage.end(), lowerMessage.begin(), ::tolower);
    
    if (lowerMessage.find("printer") != std::string::npos && 
        lowerMessage.find("not found") != std::string::npos) {
        return PrinterErrorCode::PRINTER_NOT_FOUND;
    }
    
    if (lowerMessage.find("job") != std::string::npos && 
        lowerMessage.find("not found") != std::string::npos) {
        return PrinterErrorCode::JOB_NOT_FOUND;
    }
    
    if (lowerMessage.find("file") != std::string::npos && 
        lowerMessage.find("not found") != std::string::npos) {
        return PrinterErrorCode::FILE_NOT_FOUND;
    }
    
    if (lowerMessage.find("offline") != std::string::npos ||
        lowerMessage.find("not available") != std::string::npos) {
        return PrinterErrorCode::PRINTER_OFFLINE;
    }
    
    if (lowerMessage.find("access denied") != std::string::npos ||
        lowerMessage.find("permission") != std::string::npos) {
        return PrinterErrorCode::ACCESS_DENIED;
    }
    
    if (lowerMessage.find("driver") != std::string::npos) {
        return PrinterErrorCode::DRIVER_ERROR;
    }
    
    if (lowerMessage.find("invalid") != std::string::npos ||
        lowerMessage.find("argument") != std::string::npos) {
        return PrinterErrorCode::INVALID_ARGUMENTS;
    }
    
    return PrinterErrorCode::UNKNOWN;
}

PrinterException createGenericError(const std::string& message) {
    PrinterErrorCode code = mapGenericError(message);
    return PrinterException(message, code);
}

} // namespace ErrorMappers

// Utility functions
PrinterException createPrinterNotFoundError(const std::string& printerName) {
    return PrinterException(
        "Printer '" + printerName + "' not found",
        PrinterErrorCode::PRINTER_NOT_FOUND
    );
}

PrinterException createJobNotFoundError(int jobId) {
    return PrinterException(
        "Print job " + std::to_string(jobId) + " not found",
        PrinterErrorCode::JOB_NOT_FOUND
    );
}

PrinterException createFileNotFoundError(const std::string& filename) {
    return PrinterException(
        "File '" + filename + "' not found",
        PrinterErrorCode::FILE_NOT_FOUND
    );
}

PrinterException createAccessDeniedError(const std::string& operation) {
    return PrinterException(
        "Access denied for operation: " + operation,
        PrinterErrorCode::ACCESS_DENIED
    );
}

PrinterException createInvalidArgumentsError(const std::string& details) {
    return PrinterException(
        "Invalid arguments: " + details,
        PrinterErrorCode::INVALID_ARGUMENTS
    );
}

} // namespace NodePrinter