#pragma once
#include <string>
#include <stdexcept>

namespace NodePrinter {

// Unified error codes matching TypeScript PrinterErrorCode
enum class PrinterErrorCode {
    PRINTER_NOT_FOUND,
    PRINTER_OFFLINE,
    ACCESS_DENIED,
    JOB_NOT_FOUND,
    DRIVER_ERROR,
    INVALID_ARGUMENTS,
    FILE_NOT_FOUND,
    UNSUPPORTED_FORMAT,
    UNKNOWN
};

// Convert PrinterErrorCode to string
std::string printerErrorCodeToString(PrinterErrorCode code);

// Unified PrinterException for cross-platform error handling
class PrinterException : public std::exception {
private:
    std::string message_;
    PrinterErrorCode code_;
    int platformCode_;

public:
    explicit PrinterException(
        const std::string& message, 
        PrinterErrorCode code = PrinterErrorCode::UNKNOWN,
        int platformCode = 0
    );

    const char* what() const noexcept override;
    PrinterErrorCode getCode() const noexcept;
    int getPlatformCode() const noexcept;
    std::string getFullMessage() const;
};

// Platform-specific error mappers
namespace ErrorMappers {
    // Windows error code mapping
    PrinterErrorCode mapWindowsError(unsigned long dwError, const std::string& context = "");
    PrinterException createWindowsError(const std::string& message, unsigned long dwError = 0);

    // CUPS error mapping  
    PrinterErrorCode mapCupsError(const std::string& cupsError);
    PrinterException createCupsError(const std::string& message, const std::string& cupsError = "");

    // Generic error mapping based on message content
    PrinterErrorCode mapGenericError(const std::string& message);
    PrinterException createGenericError(const std::string& message);
}

// Utility functions for creating specific error types
PrinterException createPrinterNotFoundError(const std::string& printerName);
PrinterException createJobNotFoundError(int jobId);
PrinterException createFileNotFoundError(const std::string& filename);
PrinterException createAccessDeniedError(const std::string& operation);
PrinterException createInvalidArgumentsError(const std::string& details);

} // namespace NodePrinter