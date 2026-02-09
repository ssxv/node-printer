// Print options mapping utilities
// Normalizes print options between JavaScript and native APIs

#pragma once
#include <string>
#include <map>
#include <algorithm>
#include <cctype>

namespace NodePrinter {
namespace OptionsMapping {

/**
 * Validate and normalize paper size
 */
std::string normalizePaperSize(const std::string& paperSize) {
    if (paperSize.empty()) return "";
    
    std::string normalized = paperSize;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::toupper);
    
    // Common paper size mappings
    static const std::map<std::string, std::string> sizeMap = {
        {"A4", "A4"},
        {"A3", "A3"},
        {"A5", "A5"},
        {"LETTER", "Letter"},
        {"LEGAL", "Legal"}, 
        {"LEDGER", "Ledger"},
        {"TABLOID", "Tabloid"},
        {"EXECUTIVE", "Executive"},
        {"FOLIO", "Folio"},
        {"STATEMENT", "Statement"},
        {"10X14", "10x14"},
        {"11X17", "11x17"}
    };
    
    auto it = sizeMap.find(normalized);
    return (it != sizeMap.end()) ? it->second : paperSize;
}

/**
 * Validate and normalize orientation
 */
std::string normalizeOrientation(const std::string& orientation) {
    if (orientation.empty()) return "";
    
    std::string normalized = orientation;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    
    if (normalized == "portrait" || normalized == "p") {
        return "portrait";
    } else if (normalized == "landscape" || normalized == "l") {
        return "landscape";
    }
    
    return ""; // Invalid orientation
}

/**
 * Validate copy count
 */
int validateCopies(int copies) {
    if (copies < 1) return 1;
    if (copies > 999) return 999; // Reasonable upper limit
    return copies;
}

/**
 * Convert PrintOptions to platform-specific format for Windows
 */
#ifdef _WIN32
void applyWindowsPrintOptions(const PrintOptions& options, DEVMODE* devMode) {
    if (!devMode) return;
    
    // Set copies
    if (options.copies > 1) {
        devMode->dmCopies = validateCopies(options.copies);
        devMode->dmFields |= DM_COPIES;
    }
    
    // Set duplex
    if (options.duplex) {
        devMode->dmDuplex = DMDUP_VERTICAL; // Default to long-edge duplex
        devMode->dmFields |= DM_DUPLEX;
    }
    
    // Set color
    if (options.color) {
        devMode->dmColor = DMCOLOR_COLOR;
    } else {
        devMode->dmColor = DMCOLOR_MONOCHROME;
    }
    devMode->dmFields |= DM_COLOR;
    
    // Set orientation
    if (!options.orientation.empty()) {
        if (options.orientation == "landscape") {
            devMode->dmOrientation = DMORIENT_LANDSCAPE;
        } else {
            devMode->dmOrientation = DMORIENT_PORTRAIT;
        }
        devMode->dmFields |= DM_ORIENTATION;
    }
    
    // Paper size would need additional mapping to Windows paper size constants
    // This is complex and printer-specific, so omitted for now
}
#endif

/**
 * Convert PrintOptions to CUPS options
 */
#ifdef __linux__
std::map<std::string, std::string> toCupsOptions(const PrintOptions& options) {
    std::map<std::string, std::string> cupsOptions;
    
    // Set copies
    if (options.copies > 1) {
        cupsOptions["copies"] = std::to_string(validateCopies(options.copies));
    }
    
    // Set duplex (sides)
    if (options.duplex) {
        cupsOptions["sides"] = "two-sided-long-edge";
    } else {
        cupsOptions["sides"] = "one-sided";
    }
    
    // Set color mode
    if (options.color) {
        cupsOptions["ColorModel"] = "RGB";
        cupsOptions["print-color-mode"] = "color";
    } else {
        cupsOptions["ColorModel"] = "Gray";
        cupsOptions["print-color-mode"] = "monochrome";
    }
    
    // Set orientation
    if (!options.orientation.empty()) {
        if (options.orientation == "landscape") {
            cupsOptions["orientation-requested"] = "4"; // landscape
        } else {
            cupsOptions["orientation-requested"] = "3"; // portrait
        }
    }
    
    // Set paper size
    if (!options.paperSize.empty()) {
        std::string normalizedSize = normalizePaperSize(options.paperSize);
        cupsOptions["PageSize"] = normalizedSize;
        cupsOptions["media"] = normalizedSize;
    }
    
    return cupsOptions;
}
#endif

/**
 * Create validated PrintOptions from raw input
 */
PrintOptions validatePrintOptions(const PrintOptions& input) {
    PrintOptions validated = input;
    
    // Validate copies
    validated.copies = validateCopies(input.copies);
    
    // Normalize paper size
    if (!input.paperSize.empty()) {
        validated.paperSize = normalizePaperSize(input.paperSize);
    }
    
    // Normalize orientation
    if (!input.orientation.empty()) {
        validated.orientation = normalizeOrientation(input.orientation);
    }
    
    // Validate job name (limit length)
    if (input.jobName.length() > 255) {
        validated.jobName = input.jobName.substr(0, 255);
    }
    
    return validated;
}

/**
 * Merge user options with default options
 */
PrintOptions mergeWithDefaults(const PrintOptions& userOptions) {
    PrintOptions merged;
    
    // Set defaults
    merged.copies = 1;
    merged.duplex = false;
    merged.color = false; // Default to monochrome for cost savings
    merged.orientation = "portrait";
    merged.jobName = "Node.js Print Job";
    
    // Override with user options
    if (userOptions.copies > 0) merged.copies = validateCopies(userOptions.copies);
    if (userOptions.duplex) merged.duplex = true;
    if (userOptions.color) merged.color = true;
    if (!userOptions.paperSize.empty()) merged.paperSize = normalizePaperSize(userOptions.paperSize);
    if (!userOptions.orientation.empty()) merged.orientation = normalizeOrientation(userOptions.orientation);
    if (!userOptions.jobName.empty()) merged.jobName = userOptions.jobName;
    
    return merged;
}

} // namespace OptionsMapping
} // namespace NodePrinter