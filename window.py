"""
Window manager for Google AI Overlay.
Configures and controls the WebView2 window, JavaScript bridging, and Win32 focus.
"""

import os
import urllib.parse
import webbrowser
import ctypes
import webview

from i18n import I18nManager
from inject import CLEAN_CSS, get_inject_js

# Win32 Constants
SW_HIDE = 0
SW_SHOW = 5
HWND_TOPMOST = -1
SWP_NOMOVE = 0x0002
SWP_NOSIZE = 0x0001
SWP_SHOWWINDOW = 0x0040


class JsApi:
    """
    Exposed Python API callable from JavaScript via window.pywebview.api.*
    """
    def __init__(self, manager):
        self.manager = manager

    def get_initial_state(self):
        return {
            "lang": self.manager.i18n.get_lang(),
            "strings": self.manager.i18n.get_all_strings()
        }

    def toggle_language(self):
        new_lang = self.manager.i18n.toggle_lang()
        return {
            "lang": new_lang,
            "strings": self.manager.i18n.get_all_strings()
        }

    def search(self, query: str, lang: str = None):
        self.manager.perform_search(query, lang)

    def hide_window(self):
        self.manager.hide()

    def go_home(self):
        self.manager.load_home()

    def open_external(self, url: str):
        if url:
            webbrowser.open(url)


class OverlayWindowManager:
    def __init__(self, i18n: I18nManager):
        self.i18n = i18n
        self.window = None
        self.current_query = ""
        self.is_visible = False
        self.api = JsApi(self)

        # Base path for local UI
        self.base_dir = os.path.dirname(os.path.abspath(__file__))
        self.home_url = f"file:///{os.path.join(self.base_dir, 'ui', 'index.html').replace('\\\\', '/')}"

    def create_window(self):
        self.window = webview.create_window(
            title="Google AI Overlay",
            url=self.home_url,
            js_api=self.api,
            width=780,
            height=580,
            resizable=True,
            frameless=True,
            on_top=True,
            easy_drag=True,
            background_color='#090d16'
        )
        self.window.events.loaded += self.on_page_loaded
        self.is_visible = True
        return self.window

    def on_page_loaded(self):
        if not self.window:
            return
        
        current_url = self.window.get_current_url() or ""
        if "google.com/search" in current_url:
            # Inject stylesheet and sticky modern header
            self.window.load_css(CLEAN_CSS)
            js = get_inject_js(self.current_query, self.i18n.get_lang())
            self.window.evaluate_js(js)

    def perform_search(self, query: str, lang: str = None):
        if not query:
            return
        
        if lang:
            self.i18n.set_lang(lang)

        self.current_query = query
        encoded = urllib.parse.quote_plus(query)
        target_lang = self.i18n.get_lang()
        # Force dark mode & language param
        google_url = f"https://www.google.com/search?q={encoded}&hl={target_lang}&cs=1"
        self.window.load_url(google_url)

    def load_home(self):
        self.current_query = ""
        self.window.load_url(self.home_url)

    def show(self):
        if not self.window:
            return
        
        try:
            self.window.show()
            self.is_visible = True
            self._bring_to_foreground()
        except Exception:
            pass

    def hide(self):
        if not self.window:
            return
        
        try:
            self.window.hide()
            self.is_visible = False
        except Exception:
            pass

    def toggle(self):
        if self.is_visible:
            self.hide()
        else:
            self.show()

    def _bring_to_foreground(self):
        """
        Ensures the window receives immediate keyboard focus in Windows.
        """
        try:
            hwnd = ctypes.windll.user32.FindWindowW(None, "Google AI Overlay")
            if hwnd:
                ctypes.windll.user32.ShowWindow(hwnd, SW_SHOW)
                ctypes.windll.user32.SetWindowPos(
                    hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                    SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW
                )
                ctypes.windll.user32.SetForegroundWindow(hwnd)
        except Exception:
            pass
