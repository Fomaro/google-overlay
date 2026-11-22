import os
import sys

# Ensure project root is in sys.path
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))

import pytest
from i18n import I18nManager, STRINGS, detect_system_language
from inject import get_inject_js, CLEAN_CSS
import urllib.parse

def test_i18n_basic():
    mgr = I18nManager("en")
    assert mgr.get_lang() == "en"
    assert mgr.get_string("app_title") == "Google AI Overlay"

    # Toggle to Russian
    mgr.toggle_lang()
    assert mgr.get_lang() == "ru"
    assert "Google AI" in mgr.get_string("placeholder") or "Спросите" in mgr.get_string("placeholder")

    # Toggle back to English
    mgr.toggle_lang()
    assert mgr.get_lang() == "en"

def test_i18n_strings_parity():
    # Ensure all keys in English exist in Russian
    en_keys = set(STRINGS["en"].keys())
    ru_keys = set(STRINGS["ru"].keys())
    assert en_keys == ru_keys

def test_inject_js_escaping():
    query = 'What is "Python" & how does it handle backslashes \\ and single quotes \'? Тест на русском.'
    js = get_inject_js(query, "ru")
    assert "Тест на русском" in js
    assert "currentLang = 'ru'" in js
    assert "spotlight-injected-header" in js

def test_clean_css_rules():
    assert "#header" in CLEAN_CSS
    assert "AI Overview" in CLEAN_CSS or "div[data-attrid" in CLEAN_CSS
    assert "spotlight-injected-header" in CLEAN_CSS
