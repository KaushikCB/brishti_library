/**
 * BrishtiLibrary Warm Blue Theme Manager
 * Respects initial OS preference (prefers-color-scheme)
 * Persists user manual selection in localStorage ('brishti_theme')
 * Seamlessly toggles data-theme attribute on <html>: 'light' or 'dark'
 */
(function () {
  const STORAGE_KEY = "brishti_theme";

  function getSystemPreference() {
    return window.matchMedia && window.matchMedia("(prefers-color-scheme: dark)").matches
      ? "dark"
      : "light";
  }

  function getInitialTheme() {
    try {
      const saved = localStorage.getItem(STORAGE_KEY);
      if (saved === "light" || saved === "dark") {
        return saved;
      }
    } catch (e) {
      // Storage access blocked or restricted
    }
    return getSystemPreference();
  }

  function applyTheme(theme) {
    document.documentElement.setAttribute("data-theme", theme);
    updateToggleButtons(theme);
  }

  function updateToggleButtons(theme) {
    const isDark = theme === "dark";
    const buttons = document.querySelectorAll(".theme-toggle-btn, #theme-toggle");
    buttons.forEach((btn) => {
      btn.setAttribute("aria-label", isDark ? "Switch to light theme" : "Switch to dark theme");
      btn.setAttribute("title", isDark ? "Switch to light theme" : "Switch to dark theme");
    });
  }

  // Apply immediately to prevent FOUC (flash of unstyled content)
  const currentTheme = getInitialTheme();
  document.documentElement.setAttribute("data-theme", currentTheme);

  // Global toggle function
  window.toggleTheme = function () {
    const active = document.documentElement.getAttribute("data-theme") || "light";
    const next = active === "dark" ? "light" : "dark";
    try {
      localStorage.setItem(STORAGE_KEY, next);
    } catch (e) {}
    applyTheme(next);
    return next;
  };

  window.setTheme = function (theme) {
    if (theme !== "light" && theme !== "dark") return;
    try {
      localStorage.setItem(STORAGE_KEY, theme);
    } catch (e) {}
    applyTheme(theme);
  };

  // Wire buttons on DOM load
  document.addEventListener("DOMContentLoaded", function () {
    updateToggleButtons(document.documentElement.getAttribute("data-theme") || currentTheme);

    document.addEventListener("click", function (e) {
      const btn = e.target.closest(".theme-toggle-btn, #theme-toggle");
      if (btn) {
        e.preventDefault();
        window.toggleTheme();
      }
    });

    // Listen to OS theme changes if user has not set a preference
    if (window.matchMedia) {
      window.matchMedia("(prefers-color-scheme: dark)").addEventListener("change", function (e) {
        try {
          if (!localStorage.getItem(STORAGE_KEY)) {
            applyTheme(e.matches ? "dark" : "light");
          }
        } catch (err) {}
      });
    }
  });
})();
