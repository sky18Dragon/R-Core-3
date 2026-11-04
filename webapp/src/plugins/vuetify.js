import "vuetify/styles";
import { createVuetify } from "vuetify";
import { en, zhHans } from "vuetify/locale";

// "paper": the e-ink look used by the device pages (/ and /provision) -- black, white
// and grey only. Status is never carried by colour alone (alerts, snackbars and chips
// always include an icon and words), so the semantic colours below are all greys.
// The "light" theme stays as the default for /demo, which has its own brand styling.
const paper = {
  dark: false,
  colors: {
    background: "#f7f7f7",
    surface: "#ffffff",
    "surface-variant": "#f0f0f0",
    "on-surface-variant": "#666666",
    primary: "#111111",
    "on-primary": "#ffffff",
    secondary: "#666666",
    "on-secondary": "#ffffff",
    accent: "#111111",
    error: "#111111",
    "on-error": "#ffffff",
    info: "#444444",
    "on-info": "#ffffff",
    success: "#111111",
    "on-success": "#ffffff",
    warning: "#444444",
    "on-warning": "#ffffff",
  },
  variables: {
    "border-color": "#111111",
    "border-opacity": 0.12,
    "high-emphasis-opacity": 1,
    "medium-emphasis-opacity": 0.6,
  },
};

// "paperDark": the same skin for a dark system setting. Photos read better on a
// dark page; status still never relies on colour.
const paperDark = {
  dark: true,
  colors: {
    background: "#121212",
    surface: "#1c1c1c",
    "surface-variant": "#2a2a2a",
    "on-surface-variant": "#a8a8a8",
    primary: "#f2f2f2",
    "on-primary": "#111111",
    secondary: "#a8a8a8",
    "on-secondary": "#111111",
    accent: "#f2f2f2",
    error: "#f2f2f2",
    "on-error": "#111111",
    info: "#cccccc",
    "on-info": "#111111",
    success: "#f2f2f2",
    "on-success": "#111111",
    warning: "#cccccc",
    "on-warning": "#111111",
  },
  variables: {
    "border-color": "#ffffff",
    "border-opacity": 0.14,
    "high-emphasis-opacity": 1,
    "medium-emphasis-opacity": 0.66,
  },
};

export default createVuetify({
  locale: {
    locale: "en",
    fallback: "en",
    messages: { en, zhHans },
  },
  theme: {
    defaultTheme: "light",
    themes: {
      light: {
        colors: {
          primary: "#ce9160",
          secondary: "#424242",
          accent: "#82B1FF",
          error: "#982f2f",
          info: "#2f6398",
          success: "#2f9852",
          warning: "#987e2f",
        },
      },
      paper,
      paperDark,
    },
  },
  defaults: {
    VBtn: {
      variant: "flat",
    },
    VCard: {
      elevation: 2,
    },
  },
});
