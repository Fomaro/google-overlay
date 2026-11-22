# Google AI Overlay (Windows)

A lightweight, distraction-free Spotlight overlay for Windows that delivers Google Search with AI Overview on demand. Summon it anywhere with a global shortcut, type your query, and get an immediate smart synthesis without opening a heavy browser tab.

## Features

- **Global Hotkey:** Press `Alt + Space` anytime to toggle the overlay on top of any active window.
- **AI Overview with Zero API Keys:** Leverages system WebView2 to render real-time Google AI Overviews and search cards without subscription fees or token setups.
- **Distraction-Free UI:** Automatically cleans headers, advertisements, footers, and sidebars, presenting only the core answer and relevant sources.
- **Bilingual Interface:** Detects your Windows system language (English / Russian) automatically, with an instant in-app toggle (`RU` / `EN`).
- **External Links:** Clicking reference links automatically opens them in your primary desktop browser, keeping the overlay focused.
- **System Tray:** Runs quietly in the background with quick access to visibility and language settings.

## Keyboard Shortcuts

| Shortcut | Action |
| --- | --- |
| `Alt + Space` | Summon or dismiss the overlay |
| `Enter` | Execute query |
| `Escape` | Dismiss overlay immediately |

## Getting Started

### Prerequisites

- Windows 10 / 11 with WebView2 Runtime (installed by default on modern Windows)
- Python 3.10+

### Installation

1. Clone the repository:
   ```bash
   git clone git@github.com:Fomaro/google-overlay.git
   cd google-overlay
   ```

2. Install dependencies:
   ```bash
   pip install -r requirements.txt
   ```

3. Launch the application:
   ```bash
   python main.py
   ```
   Or double-click `run.bat`.

## Architecture

- **`main.py`**: Background lifecycle, system tray orchestration, and global hotkeys.
- **`window.py`**: Window positioning, frameless TopMost management, and JavaScript bridge.
- **`inject.py`**: Dynamic DOM cleaner and style injection engine.
- **`i18n.py`**: Locale detection and localization dictionary.
- **`ui/index.html`**: Clean launcher interface with quick query suggestions.

## License

Open-source licensed under the MIT License.
