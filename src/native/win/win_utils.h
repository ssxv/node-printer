#pragma once
#ifdef _WIN32

#include <windows.h>
#include <winspool.h>
#include <string>

namespace NodePrinter {
namespace WinUtils {

// String conversion utilities
static std::string ws_to_utf8(LPCWSTR wstr) {
  if (!wstr) return std::string();  
  int len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
  if (len <= 0) return std::string();
  std::string out;
  out.resize(len - 1);
  WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &out[0], len, NULL, NULL);
  return out;
}

static std::wstring utf8_to_ws(const std::string& str) {
  if (str.empty()) return std::wstring();
  int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
  if (len <= 0) return std::wstring();
  std::wstring out;
  out.resize(len - 1);
  MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &out[0], len);
  return out;
}

// RAII wrapper for printer handle  
struct PrinterHandle {
  HANDLE h;
  BOOL ok;
  PrinterHandle(LPCWSTR name) : h(NULL), ok(FALSE) {
    ok = OpenPrinterW(const_cast<LPWSTR>(name), &h, NULL);
  }
  ~PrinterHandle() { if (ok) ClosePrinter(h); }
  operator HANDLE() { return h; }
  bool isOk() const { return !!ok; }
};

// Convert SYSTEMTIME to Unix timestamp  
static int64_t systemtime_to_unix_timestamp(const SYSTEMTIME &st) {
  FILETIME ft;
  if (!SystemTimeToFileTime(&st, &ft)) return 0;
  
  ULARGE_INTEGER uli;
  uli.HighPart = ft.dwHighDateTime;
  uli.LowPart = ft.dwLowDateTime;
  
  const uint64_t EPOCH_DIFF = 11644473600ULL; // seconds between 1601 and 1970
  uint64_t totalSeconds = uli.QuadPart / 10000000ULL;
  return (totalSeconds <= EPOCH_DIFF) ? 0 : static_cast<int64_t>(totalSeconds - EPOCH_DIFF);
}

} // namespace WinUtils
} // namespace NodePrinter

#endif // _WIN32