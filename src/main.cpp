#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <windowsx.h>
#include <initguid.h>
#include <shellapi.h>
#include <dwmapi.h>
#include <string>
#include <functional>
#include <vector>

#include "../include/WebView2.h"

// Win32 Identifiers
constexpr UINT WM_TRAYICON = WM_APP + 101;
constexpr int HOTKEY_ID = 2001;
constexpr UINT ID_TRAY_OPEN = 3001;
constexpr UINT ID_TRAY_LANG = 3002;
constexpr UINT ID_TRAY_EXIT = 3003;

// Application State
static HWND g_hWnd = nullptr;
static NOTIFYICONDATAW g_nid = {};
static bool g_isVisible = false;
static std::wstring g_currentLang = L"en";
static std::wstring g_currentQuery = L"";
static int g_currentOpacity = 96;

// WebView2 pointers
static ICoreWebView2Controller* g_controller = nullptr;
static ICoreWebView2* g_webview = nullptr;

// Concrete COM Handlers
#define IMPLEMENT_COM_BASE(ClassName, InterfaceName) \
    LONG m_ref = 1; \
public: \
    ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&m_ref); } \
    ULONG STDMETHODCALLTYPE Release() override { \
        ULONG c = InterlockedDecrement(&m_ref); \
        if (c == 0) delete this; \
        return c; \
    } \
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override { \
        if (!ppv) return E_POINTER; \
        if (riid == IID_IUnknown || riid == IID_##InterfaceName) { \
            *ppv = static_cast<InterfaceName*>(this); \
            AddRef(); \
            return S_OK; \
        } \
        *ppv = nullptr; \
        return E_NOINTERFACE; \
    }

class EnvironmentCreatedHandler : public ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler {
    IMPLEMENT_COM_BASE(EnvironmentCreatedHandler, ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler)
    std::function<HRESULT(HRESULT, ICoreWebView2Environment*)> m_cb;
public:
    EnvironmentCreatedHandler(std::function<HRESULT(HRESULT, ICoreWebView2Environment*)> cb) : m_cb(cb) {}
    HRESULT STDMETHODCALLTYPE Invoke(HRESULT res, ICoreWebView2Environment* env) override {
        return m_cb(res, env);
    }
};

class ControllerCreatedHandler : public ICoreWebView2CreateCoreWebView2ControllerCompletedHandler {
    IMPLEMENT_COM_BASE(ControllerCreatedHandler, ICoreWebView2CreateCoreWebView2ControllerCompletedHandler)
    std::function<HRESULT(HRESULT, ICoreWebView2Controller*)> m_cb;
public:
    ControllerCreatedHandler(std::function<HRESULT(HRESULT, ICoreWebView2Controller*)> cb) : m_cb(cb) {}
    HRESULT STDMETHODCALLTYPE Invoke(HRESULT res, ICoreWebView2Controller* ctrl) override {
        return m_cb(res, ctrl);
    }
};

class NewWindowHandler : public ICoreWebView2NewWindowRequestedEventHandler {
    IMPLEMENT_COM_BASE(NewWindowHandler, ICoreWebView2NewWindowRequestedEventHandler)
    std::function<HRESULT(ICoreWebView2*, ICoreWebView2NewWindowRequestedEventArgs*)> m_cb;
public:
    NewWindowHandler(std::function<HRESULT(ICoreWebView2*, ICoreWebView2NewWindowRequestedEventArgs*)> cb) : m_cb(cb) {}
    HRESULT STDMETHODCALLTYPE Invoke(ICoreWebView2* sender, ICoreWebView2NewWindowRequestedEventArgs* args) override {
        return m_cb(sender, args);
    }
};

class WebMessageHandler : public ICoreWebView2WebMessageReceivedEventHandler {
    IMPLEMENT_COM_BASE(WebMessageHandler, ICoreWebView2WebMessageReceivedEventHandler)
    std::function<HRESULT(ICoreWebView2*, ICoreWebView2WebMessageReceivedEventArgs*)> m_cb;
public:
    WebMessageHandler(std::function<HRESULT(ICoreWebView2*, ICoreWebView2WebMessageReceivedEventArgs*)> cb) : m_cb(cb) {}
    HRESULT STDMETHODCALLTYPE Invoke(ICoreWebView2* sender, ICoreWebView2WebMessageReceivedEventArgs* args) override {
        return m_cb(sender, args);
    }
};

class NavigationCompletedHandler : public ICoreWebView2NavigationCompletedEventHandler {
    IMPLEMENT_COM_BASE(NavigationCompletedHandler, ICoreWebView2NavigationCompletedEventHandler)
    std::function<HRESULT(ICoreWebView2*, ICoreWebView2NavigationCompletedEventArgs*)> m_cb;
public:
    NavigationCompletedHandler(std::function<HRESULT(ICoreWebView2*, ICoreWebView2NavigationCompletedEventArgs*)> cb) : m_cb(cb) {}
    HRESULT STDMETHODCALLTYPE Invoke(ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args) override {
        return m_cb(sender, args);
    }
};

// JSON Field Extractor
std::wstring ExtractJsonField(const std::wstring& json, const std::wstring& key) {
    std::wstring searchKey = L"\"" + key + L"\":";
    size_t pos = json.find(searchKey);
    if (pos == std::wstring::npos) {
        searchKey = L"\"" + key + L"\": ";
        pos = json.find(searchKey);
        if (pos == std::wstring::npos) return L"";
    }
    pos += searchKey.length();
    while (pos < json.length() && (json[pos] == L' ' || json[pos] == L'\t')) pos++;
    if (pos >= json.length()) return L"";

    if (json[pos] == L'\"') {
        pos++;
        std::wstring value;
        while (pos < json.length()) {
            if (json[pos] == L'\\') {
                if (pos + 1 < json.length()) {
                    pos++;
                    if (json[pos] == L'\"') value += L'\"';
                    else if (json[pos] == L'\\') value += L'\\';
                    else if (json[pos] == L'n') value += L'\n';
                    else if (json[pos] == L't') value += L'\t';
                    else if (json[pos] == L'r') value += L'\r';
                    else value += json[pos];
                }
            } else if (json[pos] == L'\"') {
                break;
            } else {
                value += json[pos];
            }
            pos++;
        }
        return value;
    } else {
        size_t end = json.find_first_of(L",}\n\r", pos);
        if (end == std::wstring::npos) end = json.length();
        return json.substr(pos, end - pos);
    }
}

// System language detection
std::wstring DetectSystemLanguage() {
    LANGID langId = GetUserDefaultUILanguage() & 0xFF;
    if (langId == 0x19) { // Russian
        return L"ru";
    }
    return L"en";
}

// URL Encoder
std::wstring UrlEncode(const std::wstring& str) {
    std::wstring encoded;
    for (wchar_t ch : str) {
        if ((ch >= L'a' && ch <= L'z') || (ch >= L'A' && ch <= L'Z') ||
            (ch >= L'0' && ch <= L'9') || ch == L'-' || ch == L'_' || ch == L'.' || ch == L'~') {
            encoded += ch;
        } else if (ch == L' ') {
            encoded += L'+';
        } else {
            char utf8[8] = {0};
            wchar_t wstr[2] = {ch, 0};
            int len = WideCharToMultiByte(CP_UTF8, 0, wstr, 1, utf8, sizeof(utf8), nullptr, nullptr);
            for (int i = 0; i < len; ++i) {
                wchar_t hex[4];
                swprintf_s(hex, L"%%%02X", static_cast<unsigned char>(utf8[i]));
                encoded += hex;
            }
        }
    }
    return encoded;
}

// Autostart with Windows Registry
void SetAutostart(bool enable) {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        if (enable) {
            wchar_t exePath[MAX_PATH];
            GetModuleFileNameW(nullptr, exePath, MAX_PATH);
            RegSetValueExW(hKey, L"GoogleOverlay", 0, REG_SZ, (BYTE*)exePath, (DWORD)((wcslen(exePath) + 1) * sizeof(wchar_t)));
        } else {
            RegDeleteValueW(hKey, L"GoogleOverlay");
        }
        RegCloseKey(hKey);
    }
}

bool IsAutostartEnabled() {
    HKEY hKey;
    bool enabled = false;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueExW(hKey, L"GoogleOverlay", nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
            enabled = true;
        }
        RegCloseKey(hKey);
    }
    return enabled;
}

// Opacity & Size
void SetWindowOpacity(int percent) {
    if (percent < 50) percent = 50;
    if (percent > 100) percent = 100;
    g_currentOpacity = percent;
    BYTE alpha = (BYTE)((percent * 255) / 100);
    SetLayeredWindowAttributes(g_hWnd, 0, alpha, LWA_ALPHA);
}

void SetWindowPresetSize(int w, int h) {
    RECT rc;
    GetWindowRect(g_hWnd, &rc);
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int x = (screenW - w) / 2;
    int y = (screenH - h) / 2;
    SetWindowPos(g_hWnd, nullptr, x, y, w, h, SWP_NOZORDER | SWP_NOACTIVATE);
}

// Forward declarations
void ShowOverlay();
void HideOverlay();
void ToggleOverlay();
void NavigateHome();
void PerformSearch(const std::wstring& query, const std::wstring& lang);

// Embedded Launcher HTML with Glowing Aura, Draggable Header, Settings Panel
static const wchar_t* LAUNCHER_HTML = LR"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <style>
    :root {
      --bg: #090e1a;
      --card-bg: #12192b;
      --input-bg: #182238;
      --border: rgba(255, 255, 255, 0.08);
      --border-focus: #3b82f6;
      --text: #f8fafc;
      --text-muted: #94a3b8;
      --text-subtle: #64748b;
      --glow-halo: 0 0 0 1px rgba(96, 165, 250, 0.4), 0 0 35px -5px rgba(59, 130, 246, 0.3), 0 25px 60px -15px rgba(0, 0, 0, 0.9);
      --radius: 20px;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; user-select: none; }
    html, body {
      height: 100vh;
      background: var(--bg);
      color: var(--text);
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
      overflow: hidden;
      display: flex;
      flex-direction: column;
      border-radius: var(--radius);
      box-shadow: var(--glow-halo);
      border: 1px solid rgba(255, 255, 255, 0.1);
      position: relative;
    }

    /* Top Drag Header */
    .header-bar {
      height: 44px;
      padding: 0 16px;
      display: flex;
      align-items: center;
      justify-content: space-between;
      background: rgba(18, 25, 43, 0.65);
      backdrop-filter: blur(16px);
      border-bottom: 1px solid var(--border);
      cursor: grab;
      user-select: none;
    }
    .header-bar:active { cursor: grabbing; }

    .brand {
      display: flex;
      align-items: center;
      gap: 8px;
      font-size: 13px;
      font-weight: 600;
      letter-spacing: -0.2px;
      pointer-events: none;
    }
    .sparkle {
      width: 14px;
      height: 14px;
      background: linear-gradient(135deg, #4285F4 0%, #9B51E0 50%, #EA4335 100%);
      -webkit-mask: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24'%3E%3Cpath d='M12 2L14.5 9.5L22 12L14.5 14.5L12 22L9.5 14.5L2 12L9.5 9.5L12 2Z'/%3E%3C/svg%3E") no-repeat center / contain;
      mask: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24'%3E%3Cpath d='M12 2L14.5 9.5L22 12L14.5 14.5L12 22L9.5 14.5L2 12L9.5 9.5L12 2Z'/%3E%3C/svg%3E") no-repeat center / contain;
    }
    .actions {
      display: flex;
      align-items: center;
      gap: 8px;
    }
    .icon-btn {
      background: rgba(255, 255, 255, 0.05);
      border: 1px solid var(--border);
      color: var(--text-muted);
      border-radius: 8px;
      width: 28px;
      height: 28px;
      cursor: pointer;
      display: flex;
      align-items: center;
      justify-content: center;
      transition: all 0.15s ease;
    }
    .icon-btn:hover {
      background: rgba(255, 255, 255, 0.12);
      color: var(--text);
    }
    .lang-chip {
      background: rgba(255, 255, 255, 0.06);
      border: 1px solid var(--border);
      color: var(--text-muted);
      border-radius: 8px;
      font-size: 11px;
      font-weight: 600;
      padding: 4px 10px;
      cursor: pointer;
      display: flex;
      align-items: center;
      gap: 4px;
      transition: all 0.15s ease;
      height: 28px;
    }
    .lang-chip:hover {
      background: rgba(255, 255, 255, 0.12);
      color: var(--text);
    }
    .lang-chip span.active { color: #60a5fa; }
    .close-btn {
      background: none;
      border: none;
      color: var(--text-subtle);
      cursor: pointer;
      width: 28px;
      height: 28px;
      border-radius: 6px;
      display: flex;
      align-items: center;
      justify-content: center;
    }
    .close-btn:hover { background: rgba(239, 68, 68, 0.2); color: #f87171; }

    /* Content Area */
    .content {
      flex: 1;
      padding: 24px 24px 16px 24px;
      display: flex;
      flex-direction: column;
      gap: 18px;
      overflow-y: auto;
    }

    /* Search Bar */
    .search-wrap {
      display: flex;
      align-items: center;
      background: var(--input-bg);
      border: 1px solid var(--border);
      border-radius: 14px;
      padding: 0 18px;
      height: 60px;
      box-shadow: 0 10px 30px rgba(0, 0, 0, 0.4);
      transition: all 0.2s cubic-bezier(0.16, 1, 0.3, 1);
    }
    .search-wrap:focus-within {
      border-color: var(--border-focus);
      box-shadow: 0 0 0 3px rgba(59, 130, 246, 0.25), 0 10px 35px rgba(0, 0, 0, 0.5);
    }
    .search-icon {
      width: 22px;
      height: 22px;
      color: var(--text-muted);
      margin-right: 14px;
      flex-shrink: 0;
    }
    .search-input {
      flex: 1;
      background: transparent;
      border: none;
      outline: none;
      font-size: 16.5px;
      color: var(--text);
      font-family: inherit;
    }
    .search-input::placeholder { color: var(--text-subtle); }
    .badge {
      background: rgba(255, 255, 255, 0.08);
      border: 1px solid var(--border);
      border-radius: 6px;
      padding: 4px 8px;
      font-size: 11px;
      color: var(--text-muted);
      cursor: pointer;
    }
    .badge:hover { background: rgba(255, 255, 255, 0.15); color: #fff; }

    /* Hero suggestions */
    .hero {
      flex: 1;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      text-align: center;
      gap: 14px;
    }
    .hero-tag {
      display: inline-flex;
      align-items: center;
      gap: 6px;
      padding: 5px 12px;
      background: rgba(66, 133, 244, 0.12);
      border: 1px solid rgba(66, 133, 244, 0.25);
      border-radius: 20px;
      font-size: 12px;
      color: #93c5fd;
    }
    .hero-title { font-size: 22px; font-weight: 600; }
    .hero-sub { font-size: 13.5px; color: var(--text-muted); max-width: 500px; line-height: 1.5; }
    .chips { display: flex; flex-wrap: wrap; justify-content: center; gap: 8px; margin-top: 8px; max-width: 620px; }
    .chip {
      background: rgba(255, 255, 255, 0.04);
      border: 1px solid var(--border);
      border-radius: 10px;
      padding: 8px 14px;
      font-size: 13px;
      color: var(--text-muted);
      cursor: pointer;
      transition: all 0.15s ease;
    }
    .chip:hover {
      background: rgba(255, 255, 255, 0.09);
      color: var(--text);
      border-color: rgba(255, 255, 255, 0.16);
      transform: translateY(-1px);
    }

    /* Footer */
    .footer {
      padding: 10px 20px;
      display: flex;
      align-items: center;
      justify-content: space-between;
      border-top: 1px solid var(--border);
      background: rgba(14, 21, 37, 0.5);
      font-size: 12px;
      color: var(--text-subtle);
    }
    .footer-hints { display: flex; align-items: center; gap: 14px; }
    .footer-hint-item { display: flex; align-items: center; gap: 6px; }

    /* Resize grip */
    .resize-grip {
      width: 14px;
      height: 14px;
      opacity: 0.4;
      display: flex;
      align-items: center;
      justify-content: center;
      cursor: nwse-resize;
    }

    /* Settings Modal Panel */
    .settings-panel {
      position: absolute;
      top: 44px;
      left: 0;
      right: 0;
      bottom: 0;
      background: rgba(9, 14, 26, 0.98);
      backdrop-filter: blur(20px);
      z-index: 100;
      display: none;
      flex-direction: column;
      padding: 24px;
      gap: 20px;
      overflow-y: auto;
    }
    .settings-panel.active { display: flex; }

    .settings-header {
      display: flex;
      align-items: center;
      justify-content: space-between;
      border-bottom: 1px solid var(--border);
      padding-bottom: 14px;
    }
    .settings-title { font-size: 18px; font-weight: 600; display: flex; align-items: center; gap: 8px; }
    .settings-group {
      display: flex;
      flex-direction: column;
      gap: 10px;
      background: var(--card-bg);
      border: 1px solid var(--border);
      border-radius: 12px;
      padding: 16px;
    }
    .settings-label { font-size: 13.5px; font-weight: 500; color: var(--text); display: flex; justify-content: space-between; align-items: center; }
    .settings-sublabel { font-size: 12px; color: var(--text-muted); }
    
    .slider-row {
      display: flex;
      align-items: center;
      gap: 12px;
      margin-top: 6px;
    }
    input[type=range] {
      flex: 1;
      accent-color: #3b82f6;
      cursor: pointer;
    }
    .preset-btns {
      display: flex;
      gap: 8px;
      margin-top: 6px;
    }
    .preset-btn {
      background: rgba(255, 255, 255, 0.05);
      border: 1px solid var(--border);
      color: var(--text-muted);
      border-radius: 8px;
      padding: 6px 12px;
      font-size: 12px;
      cursor: pointer;
      transition: all 0.15s ease;
    }
    .preset-btn:hover { background: rgba(255, 255, 255, 0.12); color: #fff; }
    .toggle-row {
      display: flex;
      align-items: center;
      justify-content: space-between;
    }
    .toggle-switch {
      position: relative;
      width: 42px;
      height: 22px;
      background: rgba(255, 255, 255, 0.15);
      border-radius: 20px;
      cursor: pointer;
      transition: all 0.2s ease;
    }
    .toggle-switch.on { background: #3b82f6; }
    .toggle-switch::after {
      content: '';
      position: absolute;
      top: 2px;
      left: 2px;
      width: 18px;
      height: 18px;
      background: #fff;
      border-radius: 50%;
      transition: all 0.2s cubic-bezier(0.16, 1, 0.3, 1);
    }
    .toggle-switch.on::after { transform: translateX(20px); }
  </style>
</head>
<body>
  <!-- Header with Drag Handler -->
  <div class="header-bar" id="header-bar">
    <div class="brand">
      <span class="sparkle"></span>
      <span id="txt-title">Google AI Overlay</span>
    </div>
    <div class="actions">
      <button class="icon-btn" id="btn-settings" onclick="toggleSettings()" title="Settings / Настройки">
        <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
          <circle cx="12" cy="12" r="3"></circle>
          <path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1 0 2.83 2 2 0 0 1-2.83 0l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-2 2 2 2 0 0 1-2-2v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 0 1-2.83 0 2 2 0 0 1 0-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1-2-2 2 2 0 0 1 2-2h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 0 1 0-2.83 2 2 0 0 1 2.83 0l.06.06a1.65 1.65 0 0 0 1.82.33H9a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 2-2 2 2 0 0 1 2 2v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 0 1 2.83 0 2 2 0 0 1 0 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 2 2 2 2 0 0 1-2 2h-.09a1.65 1.65 0 0 0-1.51 1z"></path>
        </svg>
      </button>
      <button class="lang-chip" id="lang-btn" onclick="toggleLang()">
        <span id="lbl-ru">RU</span>
        <span style="opacity: 0.3">/</span>
        <span id="lbl-en" class="active">EN</span>
      </button>
      <button class="close-btn" onclick="postAction('hide')" title="Hide (Esc)">
        <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round">
          <line x1="18" y1="6" x2="6" y2="18"></line>
          <line x1="6" y1="6" x2="18" y2="18"></line>
        </svg>
      </button>
    </div>
  </div>

  <!-- Main Content -->
  <div class="content">
    <div class="search-wrap">
      <svg class="search-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
        <circle cx="11" cy="11" r="8"></circle>
        <line x1="21" y1="21" x2="16.65" y2="16.65"></line>
      </svg>
      <input type="text" id="input" class="search-input" placeholder="Ask Google AI anything..." autofocus autocomplete="off" />
      <span class="badge" onclick="doSearch()">↵ Enter</span>
    </div>

    <div class="hero">
      <div class="hero-tag">
        <span class="sparkle"></span>
        <span>Google AI Overview • Fast & Native</span>
      </div>
      <h2 class="hero-title" id="txt-hero-title">Clean & Instant Google Search</h2>
      <p class="hero-sub" id="txt-hero-sub">
        Summoned anywhere with Alt+Space. Google AI Overview and key links without browser clutter.
      </p>
      <div class="chips" id="chips"></div>
    </div>
  </div>

  <!-- Settings Panel -->
  <div class="settings-panel" id="settings-panel">
    <div class="settings-header">
      <div class="settings-title">
        <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
          <circle cx="12" cy="12" r="3"></circle>
          <path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1 0 2.83 2 2 0 0 1-2.83 0l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-2 2 2 2 0 0 1-2-2v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 0 1-2.83 0 2 2 0 0 1 0-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1-2-2 2 2 0 0 1 2-2h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 0 1 0-2.83 2 2 0 0 1 2.83 0l.06.06a1.65 1.65 0 0 0 1.82.33H9a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 2-2 2 2 0 0 1 2 2v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 0 1 2.83 0 2 2 0 0 1 0 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 2 2 2 2 0 0 1-2 2h-.09a1.65 1.65 0 0 0-1.51 1z"></path>
        </svg>
        <span id="txt-settings-heading">Settings</span>
      </div>
      <button class="icon-btn" onclick="toggleSettings()">
        <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5">
          <line x1="18" y1="6" x2="6" y2="18"></line>
          <line x1="6" y1="6" x2="18" y2="18"></line>
        </svg>
      </button>
    </div>

    <!-- Opacity -->
    <div class="settings-group">
      <div class="settings-label">
        <span id="txt-lbl-opacity">Window Opacity</span>
        <span id="val-opacity" style="color: #60a5fa; font-weight: 600;">96%</span>
      </div>
      <div class="settings-sublabel" id="txt-sub-opacity">Adjust overlay transparency on desktop</div>
      <div class="slider-row">
        <input type="range" id="slider-opacity" min="60" max="100" value="96" oninput="changeOpacity(this.value)" />
      </div>
    </div>

    <!-- Size Presets -->
    <div class="settings-group">
      <div class="settings-label">
        <span id="txt-lbl-size">Window Size Presets</span>
      </div>
      <div class="settings-sublabel" id="txt-sub-size">Resize from edges freely, or pick a standard preset</div>
      <div class="preset-btns">
        <button class="preset-btn" onclick="setSize(720, 520)">Compact (720x520)</button>
        <button class="preset-btn" onclick="setSize(820, 600)">Standard (820x600)</button>
        <button class="preset-btn" onclick="setSize(960, 680)">Wide (960x680)</button>
      </div>
    </div>

    <!-- Autostart -->
    <div class="settings-group">
      <div class="toggle-row">
        <div>
          <div class="settings-label" id="txt-lbl-autostart">Launch on Windows Startup</div>
          <div class="settings-sublabel" id="txt-sub-autostart">Run automatically in background on login</div>
        </div>
        <div class="toggle-switch" id="switch-autostart" onclick="toggleAutostart()"></div>
      </div>
    </div>

    <!-- Hotkey Info -->
    <div class="settings-group">
      <div class="toggle-row">
        <div>
          <div class="settings-label" id="txt-lbl-hotkey">Global Hotkey</div>
          <div class="settings-sublabel" id="txt-sub-hotkey">Summons or hides the overlay anytime</div>
        </div>
        <span class="badge" style="font-size: 12px; font-weight: 600; padding: 6px 12px;">Alt + Space</span>
      </div>
    </div>
  </div>

  <!-- Bottom Bar (Cleaned, No C++ text) -->
  <div class="footer">
    <div class="footer-hints">
      <span class="footer-hint-item">
        <span class="badge">Alt + Space</span>
        <span id="txt-hint-summon">Summon</span>
      </span>
      <span class="footer-hint-item">
        <span class="badge">Esc</span>
        <span id="txt-hint-hide">Hide</span>
      </span>
      <span class="footer-hint-item">
        <span class="badge">Click Outside</span>
        <span id="txt-hint-blur">Dismisses</span>
      </span>
    </div>
    <!-- Visual Resize Grip -->
    <div class="resize-grip" title="Resize window">
      <svg width="10" height="10" viewBox="0 0 10 10" fill="none">
        <circle cx="8" cy="8" r="1.3" fill="#64748b"/>
        <circle cx="4" cy="8" r="1.3" fill="#64748b"/>
        <circle cx="8" cy="4" r="1.3" fill="#64748b"/>
      </svg>
    </div>
  </div>

  <script>
    let currentLang = 'en';
    let isAutostart = false;

    const suggestions = {
      en: [
        "Explain quantum computing simply",
        "Python list comprehension examples",
        "James Webb telescope latest discoveries",
        "Docker vs Podman key differences"
      ],
      ru: [
        "Объясни квантовые вычисления простыми словами",
        "Примеры генераторов списков в Python",
        "Последние открытия телескопа Джеймс Уэбб",
        "Главные отличия Docker и Podman"
      ]
    };

    function postAction(action, data = {}) {
      if (window.chrome && window.chrome.webview) {
        window.chrome.webview.postMessage({ action: action, ...data });
      }
    }

    // Window Dragging: Triggered when mouse is down on header bar
    document.getElementById('header-bar').addEventListener('mousedown', (e) => {
      if (e.button === 0 && !e.target.closest('button') && !e.target.closest('input')) {
        postAction('drag');
      }
    });

    function toggleSettings() {
      const panel = document.getElementById('settings-panel');
      panel.classList.toggle('active');
    }

    function changeOpacity(val) {
      document.getElementById('val-opacity').innerText = val + '%';
      postAction('set_opacity', { opacity: parseInt(val) });
    }

    function setSize(w, h) {
      postAction('set_size', { width: w, height: h });
    }

    function toggleAutostart() {
      isAutostart = !isAutostart;
      document.getElementById('switch-autostart').className = 'toggle-switch ' + (isAutostart ? 'on' : '');
      postAction('set_autostart', { enable: isAutostart });
    }

    function updateLanguageUI() {
      const isRu = currentLang === 'ru';
      document.getElementById('lbl-ru').className = isRu ? 'active' : '';
      document.getElementById('lbl-en').className = !isRu ? 'active' : '';
      
      document.getElementById('input').placeholder = isRu ? 'Спросите Google AI о чём угодно...' : 'Ask Google AI anything...';
      document.getElementById('txt-hero-title').innerText = isRu ? 'Чистый и быстрый поиск Google' : 'Clean & Instant Google Search';
      document.getElementById('txt-hero-sub').innerText = isRu ? 'Вызов по Alt+Space. AI Overview от Google и ключевые ссылки без лишнего мусора.' : 'Summoned anywhere with Alt+Space. Google AI Overview and key links without browser clutter.';
      document.getElementById('txt-hint-summon').innerText = isRu ? 'Вызов' : 'Summon';
      document.getElementById('txt-hint-hide').innerText = isRu ? 'Скрыть' : 'Hide';
      document.getElementById('txt-hint-blur').innerText = isRu ? 'Клик мимо скрывает' : 'Dismisses';

      // Settings Strings
      document.getElementById('txt-settings-heading').innerText = isRu ? 'Настройки' : 'Settings';
      document.getElementById('txt-lbl-opacity').innerText = isRu ? 'Прозрачность окна' : 'Window Opacity';
      document.getElementById('txt-sub-opacity').innerText = isRu ? 'Регулировка прозрачности оверлея на рабочем столе' : 'Adjust overlay transparency on desktop';
      document.getElementById('txt-lbl-size').innerText = isRu ? 'Размеры окна' : 'Window Size Presets';
      document.getElementById('txt-sub-size').innerText = isRu ? 'Тяните за края для любого размера или выберите готовый' : 'Resize from edges freely, or pick a standard preset';
      document.getElementById('txt-lbl-autostart').innerText = isRu ? 'Автозапуск при старте Windows' : 'Launch on Windows Startup';
      document.getElementById('txt-sub-autostart').innerText = isRu ? 'Запуск в фоне при входе в систему' : 'Run automatically in background on login';
      document.getElementById('txt-lbl-hotkey').innerText = isRu ? 'Горячая клавиша' : 'Global Hotkey';
      document.getElementById('txt-sub-hotkey').innerText = isRu ? 'Открывает или скрывает оверлей в любой момент' : 'Summons or hides the overlay anytime';

      const list = suggestions[currentLang] || suggestions.en;
      const container = document.getElementById('chips');
      container.innerHTML = '';
      list.forEach(text => {
        const chip = document.createElement('div');
        chip.className = 'chip';
        chip.innerText = text;
        chip.onclick = () => {
          document.getElementById('input').value = text;
          doSearch();
        };
        container.appendChild(chip);
      });
    }

    function toggleLang() {
      currentLang = currentLang === 'en' ? 'ru' : 'en';
      updateLanguageUI();
      postAction('set_lang', { lang: currentLang });
    }

    function doSearch() {
      const q = document.getElementById('input').value.trim();
      if (q) {
        postAction('search', { query: q, lang: currentLang });
      }
    }

    const input = document.getElementById('input');
    input.addEventListener('keydown', (e) => {
      if (e.key === 'Enter') doSearch();
      else if (e.key === 'Escape') postAction('hide');
    });

    window.addEventListener('keydown', (e) => {
      if (e.key === 'Escape') {
        const panel = document.getElementById('settings-panel');
        if (panel.classList.contains('active')) {
          panel.classList.remove('active');
        } else {
          postAction('hide');
        }
      }
    });

    if (window.chrome && window.chrome.webview) {
      window.chrome.webview.addEventListener('message', (e) => {
        try {
          const msg = e.data;
          if (msg.lang) {
            currentLang = msg.lang;
            updateLanguageUI();
          }
          if (msg.autostart !== undefined) {
            isAutostart = msg.autostart;
            document.getElementById('switch-autostart').className = 'toggle-switch ' + (isAutostart ? 'on' : '');
          }
        } catch (err) {}
      });
    }

    updateLanguageUI();
  </script>
</body>
</html>
)rawhtml";

// JavaScript injected into Google Search pages with Draggable Sticky Header
static const wchar_t* INJECT_GOOGLE_SCRIPT = LR"rawjs(
(function() {
  if (document.getElementById('spotlight-injected-header')) return;

  const style = document.createElement('style');
  style.id = 'spotlight-clean-styles';
  style.textContent = `
    html, body {
      background: #090e1a !important;
      color: #f8fafc !important;
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif !important;
      margin: 0 !important;
      padding: 0 !important;
      overflow-x: hidden !important;
      border-radius: 20px !important;
    }
    ::-webkit-scrollbar { width: 6px; }
    ::-webkit-scrollbar-track { background: transparent; }
    ::-webkit-scrollbar-thumb { background: rgba(255, 255, 255, 0.15); border-radius: 4px; }
    ::-webkit-scrollbar-thumb:hover { background: rgba(255, 255, 255, 0.3); }

    /* Hide Google clutter */
    #header, #tsf, #searchform, #fbar, #botstuff, #foot, #rhs,
    .appbar, #hdtb, .sfbg, .yg51tc, #top_nav, header, footer,
    [role="navigation"], .m7W83d, #searchform-holder, .minidiv,
    .RNNXgb, .embg, .sfbgx, .commercial-unit-desktop-rhs, .pla-unit-container,
    #easter-egg, #W4Efsb, .aajZCb, .gb_d, .gb_e, .gb_f, .g-blk,
    #extabar, .kp-blk, .AUiWPd, #gb, .gb_1, .gb_2, .gb_3, .fbar {
      display: none !important;
    }

    #main {
      padding-top: 76px !important;
      display: flex !important;
      flex-direction: column !important;
      align-items: center !important;
    }

    #cnt, #rcnt, #center_col, #rso {
      width: 100% !important;
      max-width: 760px !important;
      margin: 0 auto !important;
      padding: 0 16px !important;
      border: none !important;
    }

    /* AI Overview Card */
    div[data-attrid*="wa:"], div[data-md*="wa:"],
    div[aria-label*="AI Overview"], div[aria-label*="Обзор от ИИ"],
    .xpdopen, .YzCcNe,
    div[jscontroller]:has(> [aria-label*="AI Overview"]),
    div[jscontroller]:has(> [aria-label*="Обзор от ИИ"]) {
      background: linear-gradient(145deg, #12192b 0%, #182238 100%) !important;
      border: 1px solid rgba(96, 165, 250, 0.4) !important;
      border-radius: 16px !important;
      padding: 22px !important;
      margin: 16px 0 24px 0 !important;
      box-shadow: 0 10px 30px -10px rgba(0, 0, 0, 0.7), 0 0 25px -5px rgba(59, 130, 246, 0.2) !important;
      color: #f1f5f9 !important;
    }
    div[data-attrid*="wa:"] *, div[aria-label*="AI Overview"] *, div[aria-label*="Обзор от ИИ"] * {
      color: #f1f5f9 !important;
      background-color: transparent !important;
    }

    /* Organic cards */
    .g, .MjjYud > div {
      background: rgba(255, 255, 255, 0.03) !important;
      border: 1px solid rgba(255, 255, 255, 0.06) !important;
      border-radius: 12px !important;
      padding: 16px 18px !important;
      margin-bottom: 12px !important;
      transition: all 0.15s ease !important;
    }
    .g:hover, .MjjYud > div:hover {
      background: rgba(255, 255, 255, 0.06) !important;
      border-color: rgba(255, 255, 255, 0.14) !important;
    }
    h3, .LC20lb { color: #60a5fa !important; font-size: 16px !important; font-weight: 600 !important; }
    .VwiC3b, .yXK7lf, span, em { color: #94a3b8 !important; line-height: 1.5 !important; font-size: 13.5px !important; }
    .byrV5b, cite { color: #64748b !important; font-size: 12px !important; }

    /* Sticky Injected Top Bar */
    #spotlight-injected-header {
      position: fixed !important;
      top: 0 !important; left: 0 !important; right: 0 !important;
      height: 64px !important;
      background: rgba(9, 14, 26, 0.94) !important;
      backdrop-filter: blur(16px) !important;
      border-bottom: 1px solid rgba(255, 255, 255, 0.08) !important;
      z-index: 9999999 !important;
      display: flex !important;
      align-items: center !important;
      justify-content: center !important;
      padding: 0 16px !important;
      cursor: grab !important;
    }
    #spotlight-injected-header:active { cursor: grabbing !important; }

    .spotlight-inner {
      width: 100% !important; max-width: 760px !important;
      display: flex !important; align-items: center !important; gap: 12px !important;
    }
    .spotlight-btn-home {
      background: rgba(255, 255, 255, 0.05) !important;
      border: 1px solid rgba(255, 255, 255, 0.08) !important;
      border-radius: 10px !important;
      width: 38px !important; height: 38px !important;
      display: flex !important; align-items: center !important; justify-content: center !important;
      cursor: pointer !important; color: #94a3b8 !important; flex-shrink: 0 !important;
      transition: all 0.15s ease !important;
    }
    .spotlight-btn-home:hover { background: rgba(255, 255, 255, 0.1) !important; color: #f8fafc !important; }
    .spotlight-input-wrap {
      flex: 1 !important; display: flex !important; align-items: center !important;
      background: #182238 !important; border: 1px solid rgba(255, 255, 255, 0.1) !important;
      border-radius: 10px !important; height: 42px !important; padding: 0 12px !important;
      transition: all 0.15s ease !important;
    }
    .spotlight-input-wrap:focus-within {
      border-color: #3b82f6 !important;
      box-shadow: 0 0 0 2px rgba(59, 130, 246, 0.25) !important;
    }
    .spotlight-input {
      width: 100% !important; background: transparent !important;
      border: none !important; outline: none !important; color: #f8fafc !important;
      font-size: 15px !important; font-family: inherit !important;
    }
    .spotlight-btn-close {
      background: none !important; border: none !important; color: #64748b !important;
      cursor: pointer !important; width: 32px !important; height: 32px !important;
      border-radius: 6px !important; display: flex !important; align-items: center !important; justify-content: center !important;
    }
    .spotlight-btn-close:hover { background: rgba(239, 68, 68, 0.2) !important; color: #f87171 !important; }
  `;
  document.head.appendChild(style);

  // 2. Build Injected Top Bar
  const header = document.createElement('div');
  header.id = 'spotlight-injected-header';
  
  const params = new URLSearchParams(window.location.search);
  const q = params.get('q') || '';
  const lang = params.get('hl') || 'en';

  header.innerHTML = `
    <div class="spotlight-inner">
      <button class="spotlight-btn-home" id="spotlight-home" title="Launcher">
        <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
          <path d="M12 2L14.5 9.5L22 12L14.5 14.5L12 22L9.5 14.5L2 12L9.5 9.5L12 2Z"/>
        </svg>
      </button>
      <div class="spotlight-input-wrap">
        <input type="text" class="spotlight-input" id="spotlight-field" value="${q.replace(/"/g, '&quot;')}" placeholder="${lang === 'ru' ? 'Спросите Google AI...' : 'Ask Google AI...'}" />
      </div>
      <button class="spotlight-btn-close" id="spotlight-close" title="Hide (Esc)">
        <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round">
          <line x1="18" y1="6" x2="6" y2="18"></line>
          <line x1="6" y1="6" x2="18" y2="18"></line>
        </svg>
      </button>
    </div>
  `;
  document.body.prepend(header);

  // Drag Support on injected header
  header.addEventListener('mousedown', (e) => {
    if (e.button === 0 && !e.target.closest('button') && !e.target.closest('input')) {
      if (window.chrome && window.chrome.webview) {
        window.chrome.webview.postMessage({ action: 'drag' });
      }
    }
  });

  const field = document.getElementById('spotlight-field');
  field.focus();
  field.select();

  field.addEventListener('keydown', (e) => {
    if (e.key === 'Enter') {
      const val = field.value.trim();
      if (val && window.chrome && window.chrome.webview) {
        window.chrome.webview.postMessage({ action: 'search', query: val, lang: lang });
      }
    } else if (e.key === 'Escape') {
      if (window.chrome && window.chrome.webview) {
        window.chrome.webview.postMessage({ action: 'hide' });
      }
    }
  });

  document.getElementById('spotlight-home').onclick = () => {
    if (window.chrome && window.chrome.webview) {
      window.chrome.webview.postMessage({ action: 'home' });
    }
  };

  document.getElementById('spotlight-close').onclick = () => {
    if (window.chrome && window.chrome.webview) {
      window.chrome.webview.postMessage({ action: 'hide' });
    }
  };

  window.addEventListener('keydown', (e) => {
    if (e.key === 'Escape') {
      if (window.chrome && window.chrome.webview) {
        window.chrome.webview.postMessage({ action: 'hide' });
      }
    }
  });
})();
)rawjs";

// Overlay Actions
void ShowOverlay() {
    if (!g_hWnd) return;
    ShowWindow(g_hWnd, SW_SHOW);
    SetWindowPos(g_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    SetForegroundWindow(g_hWnd);
    if (g_controller) {
        g_controller->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
    }
    g_isVisible = true;
}

void HideOverlay() {
    if (!g_hWnd || !g_isVisible) return;
    ShowWindow(g_hWnd, SW_HIDE);
    g_isVisible = false;
}

void ToggleOverlay() {
    if (g_isVisible) {
        HideOverlay();
    } else {
        ShowOverlay();
    }
}

void NavigateHome() {
    if (g_webview) {
        g_currentQuery = L"";
        g_webview->NavigateToString(LAUNCHER_HTML);
    }
}

void PerformSearch(const std::wstring& query, const std::wstring& lang) {
    if (!g_webview || query.empty()) return;
    g_currentQuery = query;
    if (!lang.empty()) g_currentLang = lang;

    std::wstring encoded = UrlEncode(query);
    std::wstring url = L"https://www.google.com/search?q=" + encoded + L"&hl=" + g_currentLang + L"&cs=1";
    g_webview->Navigate(url.c_str());
}

// System Tray Setup with Startup Toast Notification
void SetupTrayIcon(HWND hWnd) {
    g_nid.cbSize = sizeof(NOTIFYICONDATAW);
    g_nid.hWnd = hWnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = (HICON)LoadImageW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(1), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    if (!g_nid.hIcon) {
        g_nid.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    }
    wcscpy_s(g_nid.szTip, L"Google AI Overlay (Alt+Space)");

    // Toast Notification in bottom-right corner
    g_nid.dwInfoFlags = NIIF_INFO;
    if (g_currentLang == L"ru") {
        wcscpy_s(g_nid.szInfoTitle, L"Google AI Overlay");
        wcscpy_s(g_nid.szInfo, L"Приложение запущено! Нажмите Alt + Space для быстрого поиска.");
    } else {
        wcscpy_s(g_nid.szInfoTitle, L"Google AI Overlay");
        wcscpy_s(g_nid.szInfo, L"Overlay is running! Press Alt + Space for instant search.");
    }

    Shell_NotifyIconW(NIM_ADD, &g_nid);
}

void RemoveTrayIcon() {
    Shell_NotifyIconW(NIM_DELETE, &g_nid);
}

void ShowTrayMenu(HWND hWnd) {
    POINT pt;
    GetCursorPos(&pt);
    HMENU hMenu = CreatePopupMenu();
    InsertMenuW(hMenu, 0, MF_BYPOSITION | MF_STRING, ID_TRAY_OPEN, L"Open Overlay (Alt+Space)");
    
    std::wstring langText = L"Language: " + (g_currentLang == L"ru" ? std::wstring(L"Русский") : std::wstring(L"English"));
    InsertMenuW(hMenu, 1, MF_BYPOSITION | MF_STRING, ID_TRAY_LANG, langText.c_str());
    InsertMenuW(hMenu, 2, MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);
    InsertMenuW(hMenu, 3, MF_BYPOSITION | MF_STRING, ID_TRAY_EXIT, L"Exit");

    SetForegroundWindow(hWnd);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, nullptr);
    DestroyMenu(hMenu);
}

// Window Procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_HOTKEY:
        if (wParam == HOTKEY_ID) {
            ToggleOverlay();
        }
        return 0;

    case WM_NCCALCSIZE:
        // Allow custom frameless window while retaining native resizability
        if (wParam == TRUE) {
            return 0;
        }
        break;

    case WM_NCHITTEST: {
        // 8-directional smooth border and corner resizing
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        RECT rc;
        GetWindowRect(hWnd, &rc);
        int border = 8;

        bool left = (pt.x >= rc.left && pt.x <= rc.left + border);
        bool right = (pt.x >= rc.right - border && pt.x <= rc.right);
        bool top = (pt.y >= rc.top && pt.y <= rc.top + border);
        bool bottom = (pt.y >= rc.bottom - border && pt.y <= rc.bottom);

        if (top && left) return HTTOPLEFT;
        if (top && right) return HTTOPRIGHT;
        if (bottom && left) return HTBOTTOMLEFT;
        if (bottom && right) return HTBOTTOMRIGHT;
        if (left) return HTLEFT;
        if (right) return HTRIGHT;
        if (top) return HTTOP;
        if (bottom) return HTBOTTOM;

        return HTCLIENT;
    }

    case WM_ACTIVATE:
        // Automatically hide to tray when user clicks outside the overlay window
        if (LOWORD(wParam) == WA_INACTIVE) {
            HideOverlay();
        }
        return 0;

    case WM_SIZE:
        if (g_controller) {
            RECT bounds;
            GetClientRect(hWnd, &bounds);
            g_controller->put_Bounds(bounds);
        }
        return 0;

    case WM_TRAYICON:
        if (lParam == WM_RBUTTONUP) {
            ShowTrayMenu(hWnd);
        } else if (lParam == WM_LBUTTONUP) {
            ToggleOverlay();
        }
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_TRAY_OPEN:
            ShowOverlay();
            break;
        case ID_TRAY_LANG:
            g_currentLang = (g_currentLang == L"ru") ? L"en" : L"ru";
            if (g_webview) {
                std::wstring msgJson = L"{\"lang\":\"" + g_currentLang + L"\"}";
                g_webview->PostWebMessageAsJson(msgJson.c_str());
            }
            break;
        case ID_TRAY_EXIT:
            DestroyWindow(hWnd);
            break;
        }
        return 0;

    case WM_DESTROY:
        RemoveTrayIcon();
        UnregisterHotKey(hWnd, HOTKEY_ID);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// WebView2 Initialization
void InitWebView2(HWND hWnd) {
    HMODULE hLoader = LoadLibraryW(L"WebView2Loader.dll");
    if (!hLoader) {
        MessageBoxW(hWnd, L"WebView2Loader.dll not found in application directory.", L"Error", MB_ICONERROR);
        return;
    }

    typedef HRESULT(STDAPICALLTYPE* PFN_CreateCoreWebView2EnvironmentWithOptions)(
        PCWSTR browserExecutableFolder,
        PCWSTR userDataFolder,
        ICoreWebView2EnvironmentOptions* environmentOptions,
        ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler* environment_created_handler);

    auto pfnCreate = (PFN_CreateCoreWebView2EnvironmentWithOptions)GetProcAddress(hLoader, "CreateCoreWebView2EnvironmentWithOptions");
    if (!pfnCreate) {
        MessageBoxW(hWnd, L"Failed to locate CreateCoreWebView2EnvironmentWithOptions in WebView2Loader.dll.", L"Error", MB_ICONERROR);
        return;
    }

    pfnCreate(nullptr, nullptr, nullptr,
        new EnvironmentCreatedHandler(
            [hWnd](HRESULT res, ICoreWebView2Environment* env) -> HRESULT {
                if (FAILED(res) || !env) return res;

                env->CreateCoreWebView2Controller(hWnd,
                    new ControllerCreatedHandler(
                        [hWnd](HRESULT res, ICoreWebView2Controller* controller) -> HRESULT {
                            if (FAILED(res) || !controller) return res;

                            g_controller = controller;
                            g_controller->AddRef();

                            RECT bounds;
                            GetClientRect(hWnd, &bounds);
                            g_controller->put_Bounds(bounds);
                            g_controller->put_IsVisible(TRUE);

                            g_controller->get_CoreWebView2(&g_webview);
                            if (g_webview) {
                                g_webview->AddRef();

                                // Intercept external links: open in system default browser
                                g_webview->add_NewWindowRequested(
                                    new NewWindowHandler(
                                        [](ICoreWebView2* sender, ICoreWebView2NewWindowRequestedEventArgs* args) -> HRESULT {
                                            args->put_Handled(TRUE);
                                            LPWSTR uri = nullptr;
                                            args->get_Uri(&uri);
                                            if (uri) {
                                                ShellExecuteW(nullptr, L"open", uri, nullptr, nullptr, SW_SHOWNORMAL);
                                                CoTaskMemFree(uri);
                                            }
                                            return S_OK;
                                        }
                                    ), nullptr
                                );

                                // Listen to messages from UI
                                g_webview->add_WebMessageReceived(
                                    new WebMessageHandler(
                                        [](ICoreWebView2* sender, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
                                            LPWSTR jsonMsg = nullptr;
                                            args->get_WebMessageAsJson(&jsonMsg);
                                            if (jsonMsg) {
                                                std::wstring msg = jsonMsg;
                                                CoTaskMemFree(jsonMsg);

                                                std::wstring action = ExtractJsonField(msg, L"action");

                                                if (action == L"hide") {
                                                    HideOverlay();
                                                } else if (action == L"home") {
                                                    NavigateHome();
                                                } else if (action == L"drag") {
                                                    // Window Dragging via Win32 Caption Move
                                                    ReleaseCapture();
                                                    SendMessageW(g_hWnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
                                                } else if (action == L"set_opacity") {
                                                    std::wstring opStr = ExtractJsonField(msg, L"opacity");
                                                    if (!opStr.empty()) {
                                                        SetWindowOpacity(_wtoi(opStr.c_str()));
                                                    }
                                                } else if (action == L"set_size") {
                                                    std::wstring wStr = ExtractJsonField(msg, L"width");
                                                    std::wstring hStr = ExtractJsonField(msg, L"height");
                                                    if (!wStr.empty() && !hStr.empty()) {
                                                        SetWindowPresetSize(_wtoi(wStr.c_str()), _wtoi(hStr.c_str()));
                                                    }
                                                } else if (action == L"set_autostart") {
                                                    std::wstring enableStr = ExtractJsonField(msg, L"enable");
                                                    SetAutostart(enableStr == L"true");
                                                } else if (action == L"set_lang") {
                                                    std::wstring lang = ExtractJsonField(msg, L"lang");
                                                    if (!lang.empty()) g_currentLang = lang;
                                                } else if (action == L"search") {
                                                    std::wstring query = ExtractJsonField(msg, L"query");
                                                    std::wstring lang = ExtractJsonField(msg, L"lang");
                                                    if (!query.empty()) {
                                                        PerformSearch(query, lang.empty() ? g_currentLang : lang);
                                                    }
                                                }
                                            }
                                            return S_OK;
                                        }
                                    ), nullptr
                                );

                                // Inject cleaning script on navigation
                                g_webview->add_NavigationCompleted(
                                    new NavigationCompletedHandler(
                                        [](ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args) -> HRESULT {
                                            LPWSTR uri = nullptr;
                                            sender->get_Source(&uri);
                                            if (uri) {
                                                std::wstring currentUrl = uri;
                                                CoTaskMemFree(uri);
                                                if (currentUrl.find(L"google.com/search") != std::wstring::npos) {
                                                    sender->ExecuteScript(INJECT_GOOGLE_SCRIPT, nullptr);
                                                }
                                            }
                                            return S_OK;
                                        }
                                    ), nullptr
                                );

                                // Navigate to initial launcher UI
                                NavigateHome();
                            }
                            return S_OK;
                        }
                    )
                );
                return S_OK;
            }
        )
    );
}

// Application Entry Point
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    // Initial system language
    g_currentLang = DetectSystemLanguage();

    // Register Window Class
    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"GoogleAIOverlayClass";
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    RegisterClassExW(&wc);

    // Calculate Centered Coordinates
    int width = 820;
    int height = 600;
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int posX = (screenW - width) / 2;
    int posY = (screenH - height) / 2;

    // Create Frameless Floating TopMost Window with Resizable Frame & Layered Opacity
    g_hWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
        wc.lpszClassName,
        L"Google AI Overlay",
        WS_POPUP | WS_THICKFRAME,
        posX, posY, width, height,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!g_hWnd) return 1;

    // Initial Opacity (96%)
    SetWindowOpacity(g_currentOpacity);

    // Apply Windows 11 DWM Smooth Rounded Corners (DWMWA_WINDOW_CORNER_PREFERENCE = 33, DWMWCP_ROUND = 2)
    DWORD cornerPreference = 2;
    DwmSetWindowAttribute(g_hWnd, 33, &cornerPreference, sizeof(cornerPreference));

    // Extend Frame for Native Drop Shadow
    MARGINS margins = { 1, 1, 1, 1 };
    DwmExtendFrameIntoClientArea(g_hWnd, &margins);

    // Setup Tray and Startup Notification
    SetupTrayIcon(g_hWnd);
    RegisterHotKey(g_hWnd, HOTKEY_ID, MOD_ALT, VK_SPACE);

    // Initialize WebView2 Engine
    InitWebView2(g_hWnd);

    // Message Loop
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    CoUninitialize();
    return (int)msg.wParam;
}
