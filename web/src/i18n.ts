// The editor's texts (app 0.2.90). Every language is one file in screen_manager/translations, the same file the firmware
// and the add-on read (docs/TRANSLATING.md); the editor uses its `editor` section and `_meta`, and the build leaves the
// other sections out (vite.config.ts). English is the source and comes with the page. Another language loads the first
// time the editor or a mockup needs it, and a text it doesn't have yet shows in English.
import { createI18n } from "vue-i18n";
import en from "../../screen_manager/translations/en.json";

export type LanguageMeta = { name: string; english: string; script?: string; plural: string; checked: boolean };
type Texts = { _meta: LanguageMeta; editor: Record<string, unknown> };

// Which form of a plural text ("1 hour ago | {n} hours ago") fits n, by the file's `_meta.plural`: the same rules as the
// screens (components/smart_display/screen_text_gen.py).
const few = (n: number) => n % 10 >= 2 && n % 10 <= 4 && (n % 100 < 12 || n % 100 > 14);
export const PLURAL_RULES: Record<string, (n: number) => number> = {
  one_other: (n) => (n === 1 ? 0 : 1),
  one_upto_1: (n) => (Math.abs(n) <= 1 ? 0 : 1),
  slavic_pl: (n) => (n === 1 ? 0 : few(n) ? 1 : 2),
  east_slavic: (n) => (n % 10 === 1 && n % 100 !== 11 ? 0 : few(n) ? 1 : 2),
  none: () => 0,
};

// Every other language by its code, as a chunk of its own.
const files = import.meta.glob<Texts>(
  ["../../screen_manager/translations/*.json", "!../../screen_manager/translations/en.json"], { import: "default" });
const LOADERS: Record<string, () => Promise<Texts>> = Object.fromEntries(
  Object.entries(files).map(([path, load]) => [path.slice(path.lastIndexOf("/") + 1, -".json".length), load]));
const META: Record<string, LanguageMeta> = { en: en._meta as LanguageMeta };
// A text with fewer forms than its language's rule uses its last one, as on the screens.
const RULES: Record<string, (choice: number, forms: number) => number> = {};
const addRule = (code: string) =>
  (RULES[code] = (choice, forms) => Math.min((PLURAL_RULES[META[code]?.plural] || PLURAL_RULES.one_other)(choice), forms - 1));

/** Every language the editor has, English first. */
export const languages = () => ["en", ...Object.keys(LOADERS).sort(), ...Object.keys(META).filter((code) => code !== "en" && !LOADERS[code])];
languages().forEach(addRule);

export const i18n = createI18n({
  legacy: false,
  locale: "en",
  fallbackLocale: "en",
  messages: { en: { editor: en.editor } } as Record<string, any>,
  pluralRules: RULES,
  // A language without a text falls back to English quietly. Only a key English lacks is a mistake, told while developing.
  missingWarn: false,
  fallbackWarn: false,
  missing: (locale, key) => {
    if (import.meta.env.DEV && locale === "en") console.warn(`[i18n] ${key} is not in en.json`);
  },
});
export const t = i18n.global.t;
export const editorLanguage = () => i18n.global.locale.value;
/** The `_meta` of a language once it is loaded. */
export const languageMeta = (code: string): LanguageMeta | undefined => META[code];

// Home Assistant opens the page in its ingress iframe, on its own origin, and sets its <html lang> to the language of
// the user's profile. Elsewhere (SCREEN_DEV, `npm run dev`) the browser's language, and English after that.
export function requestedLanguage(win: Window = window) {
  try {
    if (win.parent !== win) {
      const lang = win.parent.document.documentElement.lang;
      if (lang) return lang;
    }
  } catch {
    // A parent on another origin keeps its document to itself.
  }
  return win.navigator.language || "en";
}
// The language of the list that fits a code best: the same one (pt-BR), else its base language (pt), else none.
export function matchLanguage(wanted: string | null | undefined, available = languages()) {
  const find = (code: string) => available.find((own) => own.toLowerCase() === code.toLowerCase());
  const code = (wanted || "").trim();
  return code ? find(code) || find(code.split("-")[0]) : undefined;
}
export const pickLanguage = (wanted: string | null | undefined, available = languages()) => matchLanguage(wanted, available) || "en";

// A text a translator left empty shows in English, as on the screens. So does one vue-i18n can't read, such as a stray
// @ or {: it would stop the page from drawing.
function filled(node: unknown): unknown {
  if (typeof node === "string") return node.trim() ? node : undefined;
  if (Array.isArray(node)) return node.every((item) => typeof item === "string" && item.trim()) ? node : undefined;
  if (!node || typeof node !== "object") return undefined;
  const kept = Object.entries(node).map(([key, value]) => [key, filled(value)]).filter(([, value]) => value !== undefined);
  return kept.length ? Object.fromEntries(kept) : undefined;
}
function leaves(node: unknown, path: string): string[] {
  if (typeof node === "string") return [path];
  if (!node || typeof node !== "object") return [];
  return Object.entries(node).flatMap(([key, value]) => leaves(value, `${path}.${key}`));
}
/** A language's texts, as its file holds them. */
export function addLanguage(code: string, texts: Texts) {
  META[code] = texts._meta;
  if (!RULES[code]) addRule(code);
  const editor = (filled(texts.editor) || {}) as Record<string, any>;
  i18n.global.setLocaleMessage(code, { editor });
  const unreadable = leaves(editor, "editor").filter((key) => {
    try {
      t(key, {}, { locale: code });
      return false;
    } catch {
      return true;
    }
  });
  if (!unreadable.length) return;
  for (const key of unreadable) {
    const path = key.split(".").slice(1), last = path.pop()!;
    delete path.reduce((node, part) => node[part], editor)[last];
  }
  i18n.global.setLocaleMessage(code, { editor });
}
const loading: Record<string, Promise<boolean>> = {};
/** Loads a language once; true when the editor has it. */
export function loadLanguage(code: string): Promise<boolean> {
  if (META[code]) return Promise.resolve(true);
  const load = LOADERS[code];
  if (!load) return Promise.resolve(false);
  return (loading[code] ??= load().then(
    (texts) => (addLanguage(code, texts), true),
    () => (delete loading[code], false),
  ));
}
/** The editor's own language: the one of the user's Home Assistant profile, as far as ESP Screens has it. */
export async function setEditorLanguage(code = pickLanguage(requestedLanguage())) {
  const chosen = (await loadLanguage(code)) ? code : "en";
  i18n.global.locale.value = chosen;
  document.documentElement.lang = chosen;
  return chosen;
}

// How the screens write a number (app 0.2.90), as the firmware does (history_view::number): "1,234.5", "1.234,5" or
// "1 234,5". Anything that isn't a plain number stays as it is.
export type NumberStyle = "point" | "comma" | "space";
const MARKS: Record<NumberStyle, [string, string]> = { point: [".", ","], comma: [",", "."], space: [",", " "] };
export function numberText(value: string | number, style: NumberStyle) {
  const text = String(value), plain = /^(-?)(\d+)(?:\.(\d+))?$/.exec(text);
  if (!plain) return text;
  const [decimal, group] = MARKS[style] || MARKS.point;
  const whole = plain[2].replace(/\B(?=(\d{3})+$)/g, group);
  return `${plain[1]}${whole}${plain[3] !== undefined ? decimal + plain[3] : ""}`;
}

// The time and number format of the user's own Home Assistant profile, where it names one: a 12- or 24-hour clock, and
// "1,234.5" (comma_decimal), "1.234,5" (decimal_comma) or "1 234,5" (space_comma). "Follow the language", "use the
// system's" and "none" name none. Only inside Home Assistant's page, whose <home-assistant> element holds them.
const PROFILE_NUMBERS: Record<string, NumberStyle> = { comma_decimal: "point", decimal_comma: "comma", space_comma: "space" };
export function haProfile(win: Window = window): { clock?: "12" | "24"; numbers?: NumberStyle } {
  try {
    if (win.parent === win) return {};
    const locale = (win.parent.document.querySelector("home-assistant") as any)?.hass?.locale;
    const clock = ["12", "24"].includes(locale?.time_format) ? locale.time_format : undefined;
    return { ...(clock ? { clock } : {}), ...(PROFILE_NUMBERS[locale?.number_format] ? { numbers: PROFILE_NUMBERS[locale.number_format] } : {}) };
  } catch {
    return {};
  }
}

// "1, 2 and 3", the way the language writes a list (the CLDR, from the browser). The English texts write it without a
// comma before "and", as British English does.
export const andList = (items: (string | number)[], locale = editorLanguage()) =>
  new Intl.ListFormat(locale === "en" ? "en-GB" : locale, { type: "conjunction" }).format(items.map(String));
