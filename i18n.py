"""
Internationalization (i18n) module for Google AI Overlay.
Handles auto-detection of Windows system language and UI strings.
"""

import ctypes
import locale

STRINGS = {
    "en": {
        "app_title": "Google AI Overlay",
        "placeholder": "Ask Google AI anything...",
        "hint": "Enter to search  •  Esc to hide",
        "lang_name": "English",
        "lang_switch_tooltip": "Switch language / Сменить язык",
        "clear": "Clear",
        "tray_show": "Open Overlay (Alt+Space)",
        "tray_lang": "Language",
        "tray_exit": "Exit",
        "empty_title": "Google AI Spotlight",
        "empty_subtitle": "Instant web search & AI Overview in a distraction-free window.",
        "loading": "Searching and analyzing...",
        "open_in_browser": "Open in browser"
    },
    "ru": {
        "app_title": "Google AI Overlay",
        "placeholder": "Спросите Google AI о чём угодно...",
        "hint": "Enter — поиск  •  Esc — скрыть",
        "lang_name": "Русский",
        "lang_switch_tooltip": "Сменить язык / Switch language",
        "clear": "Очистить",
        "tray_show": "Открыть поиск (Alt+Space)",
        "tray_lang": "Язык",
        "tray_exit": "Выход",
        "empty_title": "Google AI Оверлей",
        "empty_subtitle": "Мгновенный поиск и AI Overview в чистом окне без лишнего мусора.",
        "loading": "Поиск и генерация ответа...",
        "open_in_browser": "Открыть в браузере"
    }
}


def detect_system_language() -> str:
    """
    Detect system UI language.
    Returns 'ru' if Russian/Cyrillic locale is active, otherwise 'en'.
    """
    try:
        # Check Win32 User Default UI Language
        lang_id = ctypes.windll.kernel32.GetUserDefaultUILanguage() & 0xFF
        # 0x19 is Russian (1049 & 0xFF == 25)
        if lang_id == 0x19:
            return "ru"
    except Exception:
        pass

    try:
        loc = locale.getdefaultlocale()[0]
        if loc and loc.lower().startswith("ru"):
            return "ru"
    except Exception:
        pass

    return "en"


class I18nManager:
    def __init__(self, initial_lang: str = None):
        self.lang = initial_lang or detect_system_language()

    def get_lang(self) -> str:
        return self.lang

    def set_lang(self, lang: str):
        if lang in STRINGS:
            self.lang = lang

    def toggle_lang(self) -> str:
        self.lang = "ru" if self.lang == "en" else "en"
        return self.lang

    def get_string(self, key: str) -> str:
        return STRINGS.get(self.lang, STRINGS["en"]).get(key, key)

    def get_all_strings(self) -> dict:
        return STRINGS.get(self.lang, STRINGS["en"])
