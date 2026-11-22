# Technical Specification: Google AI Overlay

## 1. Objective
A lightweight, keyboard-summoned floating spotlight overlay for Windows that delivers Google Search with AI Overview in a clean, distraction-free window. Zero API keys, zero bloat, instant response.

## 2. Key Capabilities & Requirements
1. **Global Hotkey & Window Management:**
   - Default hotkey: `Alt+Space` (or configurable).
   - Window behavior: Frameless, TopMost, centered floating modal (width ~780px, height ~560px), smooth rounded corners and dark theme.
   - Dismissal: Pressing `Esc` or clicking outside hides the window.
   - Background residency: Runs quietly in the Windows system tray with icon and menu (Show, Language, Exit).

2. **Zero-Key Google AI Overview Engine:**
   - Powered by native Windows WebView2 (Edge Chromium engine).
   - Custom overlay shell + deep CSS/JS injection:
     - Removes Google headers, footer, ads, sidebar clutter, doodles, and redundant search controls.
     - Centers and styles the AI Overview card and top search results.
     - Intercepts outbound link clicks and opens them in the user's default system browser, leaving the spotlight clean.

3. **Internationalization (i18n):**
   - Automatically detects system locale (Russian vs English).
   - Search queries include `hl=ru` or `hl=en` matching current language preference.
   - Interactive, sleek language toggle pill in the top bar to switch between RU and EN instantly.

4. **Code Structure & Deliverables:**
   - `google-ai-overlay/`
     - `main.py` - Application lifecycle, hotkey handler, system tray, background threads.
     - `window.py` - WebView2 window setup, Win32 window positioning and focus management.
     - `inject.py` - Clean CSS & JS payload injected into WebView2 to transform Google into a minimalist card.
     - `i18n.py` - Russian/English strings and system language detector.
     - `assets/` - Tray icon (`icon.ico` / `icon.png`).
     - `requirements.txt` - Python dependencies (`pywebview`, `pystray`, `Pillow`, `keyboard`, `pywin32`).
     - `run.bat` - Quick launch script.
     - `README.md` - Professional English documentation for GitHub.
     - `LICENSE` - MIT License.

## 3. Boundaries
- **Always:** Keep resource consumption minimal when hidden in tray; auto-focus search bar upon opening; open external web links in default system browser.
- **Never:** Require paid API keys or external server dependencies.
