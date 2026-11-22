# Google AI Overlay (Windows)

A blazing-fast, native Win32/C++ Spotlight-style floating search overlay for Windows that delivers Google Search with AI Overview on demand. Summon it anywhere with a global shortcut, search in real-time, and get AI answers without opening a heavy browser tab.

Written in pure native C++ with Microsoft WebView2 integration for instant (< 20ms) startup and minimal memory footprint.

## Features

- **Blazing Fast Native C++:** Zero Python or .NET runtime dependencies; compiled standalone executable under 1 MB.
- **Global Hotkey:** Press `Alt + Space` anywhere to immediately summon or dismiss the overlay.
- **Auto-Hide on Click Outside:** Seamlessly hides to the Windows system tray whenever you click away or lose focus.
- **Smooth Rounded Corners & Glowing Aura:** Windows 11 native DWM rounded corners combined with a glowing blue-violet border aura and translucent glass header.
- **AI Overview with Zero API Keys:** Uses native Windows WebView2 to access real-time Google AI Overviews and search cards without API keys or subscriptions.
- **Distraction-Free UI:** Automatically strips ads, banners, footers, doodles, and sidebars, highlighting only the AI synthesis and key sources.
- **Bilingual Interface:** Auto-detects Windows system language (`Russian` or `English`) with an instant interactive `RU / EN` toggle.
- **External Link Handling:** Reference citations and external websites automatically open in your default browser.
- **System Tray:** Sits quietly in the notification area with full status and quick controls.

## Keyboard Shortcuts

| Shortcut | Action |
| --- | --- |
| `Alt + Space` | Toggle overlay window |
| `Enter` | Execute query |
| `Escape` | Dismiss to system tray |
| `Click Outside` | Auto-hide overlay |

## Quick Start

### Running the Pre-compiled Executable

1. Download or clone this repository:
   ```bash
   git clone git@github.com:Fomaro/google-overlay.git
   cd google-overlay
   ```
2. Double-click `google-overlay.exe`.
3. Press `Alt + Space` to search!

### Building from Source

To compile from source using MinGW GCC:

```bash
build.bat
```

Or manually:
```bash
windres src/resource.rc -O coff -o src/resource.res
g++ -O2 -std=c++17 -municode -mwindows -static -static-libgcc -static-libstdc++ ^
    src/main.cpp src/resource.res ^
    -o google-overlay.exe ^
    -I include ^
    -lole32 -loleaut32 -luuid -ldwmapi -luser32 -lshell32 -lgdi32 -ladvapi32
```

## Project Structure

- **`src/main.cpp`**: Native Win32 window lifecycle, tray icon, global hotkey, DWM rounded corners, and WebView2 controller.
- **`src/resource.rc`**: Application icon and version metadata.
- **`include/`**: Microsoft WebView2 COM C++ headers.
- **`WebView2Loader.dll`**: Official Microsoft WebView2 loader library.
- **`build.bat`**: One-click build script.
- **`google-overlay.exe`**: Ready-to-run optimized standalone executable.

## License

Open-source licensed under the MIT License.
