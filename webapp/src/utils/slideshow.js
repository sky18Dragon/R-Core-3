import { cardsFromCron, describeCard, describeCardZh } from "./cron";

// One-line summary of the slideshow for collapsed panels,
// e.g. "Every day, every 12 hours" — the same wording as the schedule editor.
export function scheduleSummary(rules, locale) {
  const cards = cardsFromCron(Array.isArray(rules) ? rules : []);
  const describe = locale === "zh" ? describeCardZh : describeCard;
  return cards.map((card) => describe(card)).join(locale === "zh" ? "；" : "; ");
}
