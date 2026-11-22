"""
Main entry point for Google AI Overlay.
Handles system tray icon, global keyboard hotkey, and pywebview runtime.
"""

import os
import sys
import threading
from PIL import Image
import keyboard
import pystray
import webview

from i18n import I18nManager
from window import OverlayWindowManager


class GoogleAIOverlayApp:
    def __init__(self):
        self.i18n = I18nManager()
        self.window_mgr = OverlayWindowManager(self.i18n)
        self.tray_icon = None
        self.is_running = True

        # Assets
        self.base_dir = os.path.dirname(os.path.abspath(__file__))
        self.icon_path = os.path.join(self.base_dir, "assets", "icon.png")

    def setup_tray(self):
        try:
            image = Image.open(self.icon_path)
        except Exception:
            # Fallback simple 16x16 icon
            image = Image.new('RGB', (16, 16), color=(66, 133, 244))

        menu = pystray.Menu(
            pystray.MenuItem(
                text=lambda item: self.i18n.get_string("tray_show"),
                action=lambda item: self.window_mgr.show(),
                default=True
            ),
            pystray.MenuItem(
                text=lambda item: f"{self.i18n.get_string('tray_lang')}: {self.i18n.get_string('lang_name')}",
                action=self.on_tray_toggle_lang
            ),
            pystray.Menu.SEPARATOR,
            pystray.MenuItem(
                text=lambda item: self.i18n.get_string("tray_exit"),
                action=self.quit_app
            )
        )

        self.tray_icon = pystray.Icon(
            name="GoogleAIOverlay",
            icon=image,
            title="Google AI Overlay",
            menu=menu
        )

    def on_tray_toggle_lang(self, icon, item):
        self.i18n.toggle_lang()
        if self.tray_icon:
            self.tray_icon.update_menu()

    def setup_hotkey(self):
        def on_hotkey_pressed():
            self.window_mgr.toggle()

        try:
            # Global hotkey Alt+Space
            keyboard.add_hotkey('alt+space', on_hotkey_pressed, suppress=True)
        except Exception as e:
            print(f"[Warning] Could not bind Alt+Space: {e}")
            try:
                # Fallback to ctrl+shift+space
                keyboard.add_hotkey('ctrl+shift+space', on_hotkey_pressed)
                print("[Info] Bound Ctrl+Shift+Space as fallback hotkey.")
            except Exception as e2:
                print(f"[Error] Failed to register fallback hotkey: {e2}")

    def quit_app(self, icon=None, item=None):
        self.is_running = False
        try:
            keyboard.unhook_all()
        except Exception:
            pass

        if self.tray_icon:
            self.tray_icon.stop()

        if self.window_mgr.window:
            self.window_mgr.window.destroy()

        sys.exit(0)

    def run(self):
        self.setup_tray()
        self.setup_hotkey()

        # Start tray icon loop in a background daemon thread
        tray_thread = threading.Thread(target=self.tray_icon.run, daemon=True)
        tray_thread.start()

        # Create window
        self.window_mgr.create_window()

        # Start webview GUI event loop (must run on main thread)
        webview.start(debug=False)


if __name__ == "__main__":
    app = GoogleAIOverlayApp()
    app.run()
