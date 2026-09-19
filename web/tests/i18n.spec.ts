// The editor's texts (app 0.2.90): English from en.json with the page, another language when it is needed, the plural
// rules of the screens, and which language the page, the add-on's answers and the mockup speak.
import { mount } from "@vue/test-utils";
import { readdirSync, readFileSync, statSync } from "node:fs";
import { join } from "node:path";
import { afterEach, describe, expect, it, vi } from "vitest";
import en from "../../screen_manager/translations/en.json";
import { send } from "../src/api";
import TileCard from "../src/components/TileCard.vue";
import {
  addLanguage, andList, haProfile, i18n, languages, loadLanguage, matchLanguage, numberText, pickLanguage, PLURAL_RULES, requestedLanguage,
  setEditorLanguage, t,
} from "../src/i18n";
import { agoText, dateText } from "../src/model/topbar";
import { state } from "../src/store";

const meta = (plural: string) => ({ name: "Test", english: "Test", script: "latin", plural, checked: false });
// Every text of a section as [key, text]; a list's items get .0, .1, ... as in tools/i18n.py.
function flat(node: unknown, path: string): [string, unknown][] {
  if (node && typeof node === "object") return Object.entries(node).flatMap(([key, value]) => flat(value, `${path}.${key}`));
  return [[path, node]];
}
const english = flat(en.editor, "editor");

afterEach(() => {
  i18n.global.locale.value = "en";
  vi.unstubAllGlobals();
});

describe("English", () => {
  it("comes with the page, as the language itself and the editor's texts only", () => {
    // The build keeps the screens' and the add-on's sections out (vite.config.ts); the tests import it the same way.
    expect(Object.keys(en)).toEqual(["_meta", "editor"]);
    expect(en._meta.plural).toBe("one_other");
    expect(languages()[0]).toBe("en");
  });
  it("has a readable text for every key, and no text vue-i18n reads as something else", () => {
    for (const [key, text] of english) {
      expect(typeof text, key).toBe("string");
      expect(t(key), key).not.toBe(key);
      // @ links another text and | separates plural forms: a text uses them only as that.
      expect(text as string, key).not.toMatch(/@|\$|\|\|/);
      if ((text as string).includes("|")) expect(text as string, key).toMatch(/ \| /);
    }
  });
  it("has every key the code asks for, and no key the code doesn't use", () => {
    const root = join(__dirname, "..", "src");
    const files = (dir: string): string[] => readdirSync(dir).flatMap((name) => {
      const path = join(dir, name);
      return statSync(path).isDirectory() ? files(path) : /\.(ts|vue)$/.test(name) ? [path] : [];
    });
    const code = files(root).map((path) => readFileSync(path, "utf8")).join("\n");
    const keys = new Set([...code.matchAll(/["'`](editor\.[a-z0-9_.]+[a-z0-9_])["'`]/g)].map((m) => m[1]));
    // A key built at run time, such as `editor.library.filters.${value}`, counts for every text under its start.
    const starts = new Set([...code.matchAll(/`(editor\.[a-z0-9_.]+)\$\{/g)].map((m) => m[1]));
    const known = new Set(english.map(([key]) => key));
    for (const key of keys) expect(known.has(key) || english.some(([other]) => other.startsWith(`${key}.`)), key).toBe(true);
    for (const start of starts) expect(english.some(([key]) => key.startsWith(start)), start).toBe(true);
    const unused = english.map(([key]) => key).filter((key) => !keys.has(key) && ![...starts].some((start) => key.startsWith(start)));
    expect(unused).toEqual([]);
  });
});

describe("choosing a language", () => {
  const available = ["en", "nl", "pt-BR", "zh-Hans"];
  it("takes the same code, else its base language, else English", () => {
    expect(matchLanguage("nl", available)).toBe("nl");
    expect(matchLanguage("PT-br", available)).toBe("pt-BR");
    expect(matchLanguage("zh-hans", available)).toBe("zh-Hans");
    expect(matchLanguage("nl-BE", available)).toBe("nl");
    expect(matchLanguage("fr", available)).toBeUndefined();
    expect(pickLanguage("fr", available)).toBe("en");
    expect(pickLanguage("pt", available)).toBe("en");
    expect(pickLanguage(null, available)).toBe("en");
    expect(pickLanguage("", available)).toBe("en");
  });
  it("reads Home Assistant's <html lang> inside its iframe, else the browser's language", () => {
    const frame = document.createElement("iframe");
    document.body.append(frame);
    const inner = frame.contentWindow!;
    document.documentElement.lang = "nl";
    expect(requestedLanguage(inner)).toBe("nl");
    document.documentElement.lang = "";
    expect(requestedLanguage(inner)).toBe(inner.navigator.language);
    expect(requestedLanguage(window)).toBe(navigator.language);
    // A parent on another origin throws on its document.
    const foreign = { get parent(): Window { throw new Error("cross-origin"); }, navigator: { language: "de-DE" } } as unknown as Window;
    expect(requestedLanguage(foreign)).toBe("de-DE");
    frame.remove();
  });
  it("sets the page's own language, and keeps English for a language it doesn't have", async () => {
    addLanguage("nl", { _meta: meta("one_other"), editor: { common: { close: "Sluiten" } } });
    expect(await setEditorLanguage("nl")).toBe("nl");
    expect(document.documentElement.lang).toBe("nl");
    expect(t("editor.common.close")).toBe("Sluiten");
    // A text Dutch doesn't have yet shows in English.
    expect(t("editor.common.cancel")).toBe("Cancel");
    expect(await setEditorLanguage("xx")).toBe("en");
    expect(document.documentElement.lang).toBe("en");
    expect(await loadLanguage("xx")).toBe(false);
  });
});

describe("plural forms", () => {
  it("follow the rules of the screens", () => {
    const forms = (rule: string, numbers: number[]) => numbers.map(PLURAL_RULES[rule]);
    expect(forms("one_other", [0, 1, 2, 11])).toEqual([1, 0, 1, 1]);
    expect(forms("one_upto_1", [0, 1, 2, 11])).toEqual([0, 0, 1, 1]);
    expect(forms("slavic_pl", [1, 2, 4, 5, 12, 14, 21, 22, 25, 102])).toEqual([0, 1, 1, 2, 2, 2, 2, 1, 2, 1]);
    expect(forms("east_slavic", [1, 2, 5, 11, 12, 21, 22, 25, 111])).toEqual([0, 1, 2, 2, 2, 0, 1, 2, 2]);
    expect(forms("none", [0, 1, 2, 5])).toEqual([0, 0, 0, 0]);
  });
  it("pick the form of a text by its language's rule, with {n}, and the last form when a text has fewer", () => {
    expect(t("editor.library.entities", 1)).toBe("1 entity");
    expect(t("editor.library.entities", 5)).toBe("5 entities");
    addLanguage("pl", {
      _meta: meta("slavic_pl"),
      editor: { library: { entities: "{n} encja | {n} encje | {n} encji" }, common: { results: "{n} wynik | {n} wyników" } },
    });
    const pl = (key: string, n: number) => t(key, n, { locale: "pl" });
    expect([1, 3, 5, 22].map((n) => pl("editor.library.entities", n))).toEqual(["1 encja", "3 encje", "5 encji", "22 encje"]);
    expect(pl("editor.common.results", 3)).toBe("3 wyników");
    // Chinese has one form: every number takes it.
    addLanguage("zh-Hans", { _meta: meta("none"), editor: { library: { entities: "{n} 个实体" } } });
    expect(t("editor.library.entities", 7, { locale: "zh-Hans" })).toBe("7 个实体");
  });
  it("leave out a text that is empty or that vue-i18n can't read, and show English for it", () => {
    addLanguage("fr", {
      _meta: meta("one_upto_1"),
      editor: { common: { close: "Fermer", cancel: " ", back: "Retour @ la page" }, library: { entities: "{n} entité | {n} entités" } },
    });
    const fr = (key: string, n?: number) => (n === undefined ? t(key, {}, { locale: "fr" }) : t(key, n, { locale: "fr" }));
    expect(fr("editor.common.close")).toBe("Fermer");
    expect(fr("editor.common.cancel")).toBe("Cancel");
    expect(fr("editor.common.back")).toBe("← Back");
    expect([0, 1, 2].map((n) => fr("editor.library.entities", n))).toEqual(["0 entité", "1 entité", "2 entités"]);
  });
});

describe("the add-on hears the editor's language", () => {
  it("in a header on every request", async () => {
    const fetch = vi.fn(async () => new Response("", { status: 200 }));
    vi.stubGlobal("fetch", fetch);
    await send("screens/x/identify", "POST");
    addLanguage("nl", { _meta: meta("one_other"), editor: {} });
    i18n.global.locale.value = "nl";
    await send("inventory", "GET");
    const headers = fetch.mock.calls.map((call: any[]) => call[1].headers["X-ESP-Screens-Language"]);
    expect(headers).toEqual(["en", "nl"]);
  });
  it("and a failed answer without the add-on's own words says so in the editor's language", async () => {
    addLanguage("nl", { _meta: meta("one_other"), editor: { api: { failed: "Dat lukte niet." } } });
    i18n.global.locale.value = "nl";
    vi.stubGlobal("fetch", vi.fn(async () => new Response("<html>502</html>", { status: 502 })));
    await expect(send("screens/x", "PUT", {})).rejects.toThrow("Dat lukte niet.");
  });
});

describe("the mockup speaks the screens' language", () => {
  it("draws the screens' words in their language while the editor keeps its own", async () => {
    addLanguage("de", {
      _meta: meta("one_other"),
      editor: {
        mockup: {
          unavailable: "Nicht verfügbar", page_link: "Seite {page} ›",
          time: { hours_ago: "Vor 1 Stunde | Vor {n} Stunden", just_now: "Gerade eben" },
          date: { weekdays_min: ["So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"], months_short: ["Jan.", "Feb.", "März", "Apr.", "Mai", "Juni", "Juli", "Aug.", "Sept.", "Okt.", "Nov.", "Dez."], top_bar: "{weekday}., {day}. {month}" },
        },
        tile_card: { remove: "Kachel entfernen" },
      },
    });
    state.inventory = { screens: [], entities: [{ id: "light.b", name: "Lamp B", state: "unavailable" }], language: { setting: "de", effective: "de", ha: "en", languages: [] } } as any;
    state.layout = { title: "Living room", tiles: [] };
    state.liveStates = {};
    await Promise.resolve();
    const gone = { entity: "light.b", name: "", slot: 0 };
    state.layout.tiles.push(gone);
    const card = mount(TileCard, { props: { tile: gone, slot: 0 } });
    expect(card.find(".st").text()).toBe("Nicht verfügbar");
    // The editor's own words stay in the editor's language.
    expect(card.find(".remove").attributes("title")).toBe("Remove tile");
    const page = { entity: "screen.page_2", name: "Go to page 2", slot: 1 };
    state.layout.tiles.push(page);
    expect(mount(TileCard, { props: { tile: page, slot: 1 } }).find(".goto").text()).toBe("Seite 2 ›");
    expect(agoText(1_000_000 - 7200, 1_000_000, "de")).toBe("Vor 2 Stunden");
    expect(agoText(1_000_000 - 7200, 1_000_000)).toBe("2 hours ago");
    expect(dateText(new Date(2026, 8, 15), "de")).toBe("Di., 15. Sept.");
    // A word German doesn't have yet is English, as on the screen.
    expect(agoText(1_000_000 - 90000, 1_000_000, "de")).toBe("Yesterday");
  });
});

describe("numbers, lists and the profile", () => {
  it("writes numbers as the screens do", () => {
    expect(["1249", "21.4", "-1234567.25", "0", "999"].map((n) => numberText(n, "point"))).toEqual(["1,249", "21.4", "-1,234,567.25", "0", "999"]);
    expect(["1249", "21.4", "-1234567.25"].map((n) => numberText(n, "comma"))).toEqual(["1.249", "21,4", "-1.234.567,25"]);
    expect(["1249", "21.4"].map((n) => numberText(n, "space"))).toEqual(["1 249", "21,4"]);
    // Anything that isn't a plain number stays as it is.
    expect(["on", "1e5", "12:30", ""].map((n) => numberText(n, "comma"))).toEqual(["on", "1e5", "12:30", ""]);
    expect(numberText(1234.5, "comma")).toBe("1.234,5");
  });
  it("writes a list the way the language does, the English one without a comma before and", () => {
    expect(andList([1, 2, 3])).toBe("1, 2 and 3");
    expect(andList([2, 3], "nl")).toBe("2 en 3");
  });
  it("reads the clock and number format of the user's Home Assistant profile, where it names one", () => {
    const frame = document.createElement("iframe");
    document.body.append(frame);
    const inner = frame.contentWindow!;
    const ha = document.createElement("home-assistant") as any;
    document.body.append(ha);
    ha.hass = { locale: { language: "nl", time_format: "24", number_format: "decimal_comma" } };
    expect(haProfile(inner)).toEqual({ clock: "24", numbers: "comma" });
    ha.hass = { locale: { time_format: "12", number_format: "comma_decimal" } };
    expect(haProfile(inner)).toEqual({ clock: "12", numbers: "point" });
    ha.hass = { locale: { time_format: "language", number_format: "space_comma" } };
    expect(haProfile(inner)).toEqual({ numbers: "space" });
    ha.hass = { locale: { time_format: "system", number_format: "none" } };
    expect(haProfile(inner)).toEqual({});
    // Outside Home Assistant's page there is no profile.
    expect(haProfile(window)).toEqual({});
    ha.remove();
    frame.remove();
  });
});
