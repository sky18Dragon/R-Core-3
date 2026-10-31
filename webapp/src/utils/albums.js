import { t } from "../i18n";

// The device's built-in album is always named "Default"; show it in the user's
// language. The stored name (used in API paths) never changes.
export function albumLabel(name) {
  return name === "Default" ? t("Default album") : name;
}
