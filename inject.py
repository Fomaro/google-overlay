"""
Injection module for Google AI Overlay.
Injects custom CSS and JavaScript into Google Search to transform it into a
clean, distraction-free Spotlight overlay with AI Overview.
"""

CLEAN_CSS = """
/* === GOOGLE AI OVERLAY CLEAN STYLESHEET === */

/* 1. Global Reset & Dark Theme */
html, body {
  background-color: #090d16 !important;
  color: #f8fafc !important;
  font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif !important;
  margin: 0 !important;
  padding: 0 !important;
  overflow-x: hidden !important;
  overflow-y: overlay !important;
  user-select: text !important;
}

/* Custom modern scrollbar */
::-webkit-scrollbar {
  width: 6px;
}
::-webkit-scrollbar-track {
  background: transparent;
}
::-webkit-scrollbar-thumb {
  background: rgba(255, 255, 255, 0.15);
  border-radius: 4px;
}
::-webkit-scrollbar-thumb:hover {
  background: rgba(255, 255, 255, 0.3);
}

/* 2. Hide Google Clutter */
#header, #tsf, #searchform, #fbar, #botstuff, #foot, #rhs,
.appbar, #hdtb, .sfbg, .yg51tc, #top_nav, header, footer,
[role="navigation"], .m7W83d, #searchform-holder, .minidiv,
.RNNXgb, .embg, .sfbgx, .commercial-unit-desktop-rhs, .pla-unit-container,
#easter-egg, #W4Efsb, .aajZCb, .gb_d, .gb_e, .gb_f, .g-blk,
#extabar, .kp-blk, .AUiWPd, #searchform, .sfbg,
#gb, .gb_1, .gb_2, .gb_3, .gb_Z, .fbar {
  display: none !important;
}

/* 3. Center and Restructure Main Content */
#main {
  padding-top: 76px !important; /* Space for sticky injected header */
  display: flex !important;
  flex-direction: column !important;
  align-items: center !important;
}

#cnt, #rcnt, #center_col, #rso {
  width: 100% !important;
  max-width: 740px !important;
  margin: 0 auto !important;
  padding-left: 0 !important;
  padding-right: 0 !important;
  border: none !important;
}

/* 4. AI Overview Block Styling */
/* Selector targeting AI Overview containers in modern Google Search */
div[data-attrid*="wa:"],
div[data-md*="wa:"],
div[aria-label*="AI Overview"],
div[aria-label*="Обзор от ИИ"],
.xpdopen,
.YzCcNe,
div[jscontroller]:has(> [aria-label*="AI Overview"]),
div[jscontroller]:has(> [aria-label*="Обзор от ИИ"]) {
  background: linear-gradient(145deg, #131b2e 0%, #172138 100%) !important;
  border: 1px solid rgba(66, 133, 244, 0.35) !important;
  border-radius: 16px !important;
  padding: 20px !important;
  margin: 16px 0 24px 0 !important;
  box-shadow: 0 10px 30px -10px rgba(0, 0, 0, 0.6), 0 0 20px -5px rgba(66, 133, 244, 0.15) !important;
  color: #f1f5f9 !important;
}

/* AI Overview text & links */
div[data-attrid*="wa:"] *,
div[aria-label*="AI Overview"] *,
div[aria-label*="Обзор от ИИ"] * {
  color: #f1f5f9 !important;
  background-color: transparent !important;
}

/* 5. Organic Result Cards */
.g, .MjjYud > div {
  background: rgba(255, 255, 255, 0.03) !important;
  border: 1px solid rgba(255, 255, 255, 0.06) !important;
  border-radius: 12px !important;
  padding: 16px 18px !important;
  margin-bottom: 12px !important;
  transition: all 0.15s ease !important;
}

.g:hover, .MjjYud > div:hover {
  background: rgba(255, 255, 255, 0.05) !important;
  border-color: rgba(255, 255, 255, 0.12) !important;
}

/* Link Titles */
h3, .LC20lb {
  color: #60a5fa !important;
  font-size: 16px !important;
  font-weight: 600 !important;
}

/* Snippets & text */
.VwiC3b, .yXK7lf, span, em {
  color: #94a3b8 !important;
  line-height: 1.5 !important;
  font-size: 13.5px !important;
}

/* URL breadcrumbs */
.byrV5b, cite {
  color: #64748b !important;
  font-size: 12px !important;
}

/* 6. Sticky Injected Header */
#spotlight-injected-header {
  position: fixed !important;
  top: 0 !important;
  left: 0 !important;
  right: 0 !important;
  height: 64px !important;
  background: rgba(9, 13, 22, 0.92) !important;
  backdrop-filter: blur(16px) !important;
  border-bottom: 1px solid rgba(255, 255, 255, 0.08) !important;
  z-index: 9999999 !important;
  display: flex !important;
  align-items: center !important;
  justify-content: center !important;
  padding: 0 16px !important;
}

.spotlight-bar-inner {
  width: 100% !important;
  max-width: 740px !important;
  display: flex !important;
  align-items: center !important;
  gap: 12px !important;
}

.spotlight-btn-home {
  background: rgba(255, 255, 255, 0.05) !important;
  border: 1px solid rgba(255, 255, 255, 0.08) !important;
  border-radius: 10px !important;
  width: 38px !important;
  height: 38px !important;
  display: flex !important;
  align-items: center !important;
  justify-content: center !important;
  cursor: pointer !important;
  color: #94a3b8 !important;
  flex-shrink: 0 !important;
  transition: all 0.15s ease !important;
}

.spotlight-btn-home:hover {
  background: rgba(255, 255, 255, 0.1) !important;
  color: #f8fafc !important;
}

.spotlight-input-wrap {
  flex: 1 !important;
  position: relative !important;
  display: flex !important;
  align-items: center !important;
  background: #1e293b !important;
  border: 1px solid rgba(255, 255, 255, 0.1) !important;
  border-radius: 10px !important;
  height: 42px !important;
  padding: 0 12px !important;
  transition: all 0.15s ease !important;
}

.spotlight-input-wrap:focus-within {
  border-color: #3b82f6 !important;
  box-shadow: 0 0 0 2px rgba(59, 130, 246, 0.25) !important;
}

.spotlight-input {
  width: 100% !important;
  background: transparent !important;
  border: none !important;
  outline: none !important;
  color: #f8fafc !important;
  font-size: 15px !important;
  font-family: inherit !important;
}

.spotlight-lang-pill {
  background: rgba(255, 255, 255, 0.06) !important;
  border: 1px solid rgba(255, 255, 255, 0.1) !important;
  color: #94a3b8 !important;
  border-radius: 8px !important;
  font-size: 11px !important;
  font-weight: 600 !important;
  padding: 6px 10px !important;
  cursor: pointer !important;
  flex-shrink: 0 !important;
  display: flex !important;
  align-items: center !important;
  gap: 4px !important;
}

.spotlight-lang-pill:hover {
  background: rgba(255, 255, 255, 0.12) !important;
  color: #f8fafc !important;
}

.spotlight-btn-close {
  background: none !important;
  border: none !important;
  color: #64748b !important;
  cursor: pointer !important;
  width: 32px !important;
  height: 32px !important;
  border-radius: 6px !important;
  display: flex !important;
  align-items: center !important;
  justify-content: center !important;
  flex-shrink: 0 !important;
}

.spotlight-btn-close:hover {
  background: rgba(239, 68, 68, 0.15) !important;
  color: #f87171 !important;
}
"""

JS_TEMPLATE = """
(function() {
  if (document.getElementById('spotlight-injected-header')) return;

  const currentLang = '__LANG__';
  const currentQuery = '__QUERY__';

  const header = document.createElement('div');
  header.id = 'spotlight-injected-header';
  header.innerHTML = `
    <div class="spotlight-bar-inner">
      <button class="spotlight-btn-home" id="btn-home" title="Launcher / Домой">
        <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
          <path d="M12 2L14.5 9.5L22 12L14.5 14.5L12 22L9.5 14.5L2 12L9.5 9.5L12 2Z"/>
        </svg>
      </button>
      <div class="spotlight-input-wrap">
        <input type="text" class="spotlight-input" id="spotlight-search-field" value="${currentQuery}" placeholder="${currentLang === 'ru' ? 'Спросите Google AI...' : 'Ask Google AI...'}" />
      </div>
      <button class="spotlight-lang-pill" id="btn-lang" title="Toggle Language">
        <span style="color: ${currentLang === 'ru' ? '#60a5fa' : '#94a3b8'}">RU</span>
        <span style="opacity: 0.3">/</span>
        <span style="color: ${currentLang === 'en' ? '#60a5fa' : '#94a3b8'}">EN</span>
      </button>
      <button class="spotlight-btn-close" id="btn-close" title="Hide (Esc)">
        <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round">
          <line x1="18" y1="6" x2="6" y2="18"></line>
          <line x1="6" y1="6" x2="18" y2="18"></line>
        </svg>
      </button>
    </div>
  `;
  document.body.prepend(header);

  const input = document.getElementById('spotlight-search-field');
  if (input) {
    input.focus();
    input.select();

    input.addEventListener('keydown', (e) => {
      if (e.key === 'Enter') {
        const q = input.value.trim();
        if (q && window.pywebview && window.pywebview.api) {
          window.pywebview.api.search(q, currentLang);
        }
      } else if (e.key === 'Escape') {
        if (window.pywebview && window.pywebview.api) {
          window.pywebview.api.hide_window();
        }
      }
    });
  }

  const btnHome = document.getElementById('btn-home');
  if (btnHome) {
    btnHome.onclick = () => {
      if (window.pywebview && window.pywebview.api) {
        window.pywebview.api.go_home();
      }
    };
  }

  const btnLang = document.getElementById('btn-lang');
  if (btnLang) {
    btnLang.onclick = () => {
      if (window.pywebview && window.pywebview.api) {
        window.pywebview.api.toggle_language().then(res => {
          const q = (input ? input.value.trim() : '') || currentQuery;
          window.pywebview.api.search(q, res.lang);
        });
      }
    };
  }

  const btnClose = document.getElementById('btn-close');
  if (btnClose) {
    btnClose.onclick = () => {
      if (window.pywebview && window.pywebview.api) {
        window.pywebview.api.hide_window();
      }
    };
  }

  window.addEventListener('keydown', (e) => {
    if (e.key === 'Escape') {
      if (window.pywebview && window.pywebview.api) {
        window.pywebview.api.hide_window();
      }
    }
  });

  document.addEventListener('click', (e) => {
    const link = e.target.closest('a');
    if (!link || !link.href) return;
    
    const url = link.href;
    if (url.startsWith('http://') || url.startsWith('https://')) {
      if (!url.includes('/search?') && !url.includes('google.com/search')) {
        e.preventDefault();
        e.stopPropagation();
        if (window.pywebview && window.pywebview.api) {
          window.pywebview.api.open_external(url);
        }
      }
    }
  }, true);
})();
"""


def get_inject_js(current_query: str, current_lang: str) -> str:
    """
    Returns the JavaScript code to inject into Google Search pages.
    """
    escaped_query = current_query.replace("\\", "\\\\").replace("'", "\\'").replace('"', '\\"')
    return JS_TEMPLATE.replace("__QUERY__", escaped_query).replace("__LANG__", current_lang)
