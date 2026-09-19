// ---- The top bar as the screen draws it ----
// One rule set with header_bar.h in the firmware, in screen pixels of the board: every value (the
// time too) in the same 400 Roboto on the name's baseline; icons and the dial centred on the
// height of the digits; the gaps measured between what you see (glyph ink), not between boxes, so
// an icon with side bearings sits exactly as close to its value as one without.
import { t } from "../i18n";
import type { HeaderItem } from "../types";

// The words are the screens' own (app 0.2.90): screen.date and screen.time of the translations, in the language the
// screens have, which `locale` names; English where none is given.
export const BUILTIN_ICONS: Record<string, string> = { clock: "clock-outline", analog: "clock-outline", date: "calendar" };
export const itemKey = (item: HeaderItem) => JSON.stringify(item);
export const glyph = (cp: string) => String.fromCodePoint(parseInt(cp, 16));

export function clockText(clock24: boolean, now = new Date()) {
  let hours = now.getHours();
  // The screen formats with %I:%M when the 24-hour clock is off.
  if (!clock24) hours = hours % 12 || 12;
  return `${String(hours).padStart(2, "0")}:${String(now.getMinutes()).padStart(2, "0")}`;
}
export const dateText = (now = new Date(), locale = "en") => t("screen.date.top_bar", {
  weekday: t(`screen.date.weekdays_min.${now.getDay()}`, {}, { locale }),
  day: now.getDate(),
  month: t(`screen.date.months_short.${now.getMonth()}`, {}, { locale }),
}, { locale });
// Same thresholds as header_bar::ago_text() in the firmware, and the same words.
export function agoText(then: number, now = Math.floor(Date.now() / 1000), locale = "en") {
  const seconds = now - then, span = Math.abs(seconds), per = (unit: number) => Math.floor(span / unit);
  const say = (key: string, n?: number) => (n === undefined ? t(`screen.time.${key}`, {}, { locale }) : t(`screen.time.${key}`, n, { locale }));
  if (seconds < 0) {
    if (span < 3600) return say("in_minutes", Math.max(1, per(60)));
    if (span < 86400) return say("in_hours", per(3600));
    if (span < 172800) return say("tomorrow");
    return say("in_days", per(86400));
  }
  if (span < 60) return say("just_now");
  if (span < 3600) return say("minutes_ago", per(60));
  if (span < 86400) return say("hours_ago", per(3600));
  if (span < 172800) return say("yesterday");
  if (span < 604800) return say("days_ago", per(86400));
  if (span < 2592000) return say("weeks_ago", per(604800));
  if (span < 31536000) return say("months_ago", per(2592000));
  return say("years_ago", per(31536000));
}

export type BarMetrics = { width: number; top: number; name: number; text: number; icon: number };
export const BAR_METRICS: Record<string, BarMetrics> = {
  guition: { width: 448, top: 36, name: 27, text: 21, icon: 26 },
  cyd: { width: 298, top: 24, name: 18, text: 14, icon: 18 },
};
export type ItemView = { icon?: string | null; text?: string; color?: string | null; shown: boolean; analog?: boolean; loading?: boolean };
type Ink = { left: number; right: number; top: number; bottom: number; advance: number };

let measure: CanvasRenderingContext2D | null | undefined;
const inkCache = new Map<string, Ink>();
export const clearInkCache = () => inkCache.clear();
// Ink box of a string relative to its origin on the baseline: left/right, and top (negative, up)/bottom.
// Without a canvas (a test runner) the box is estimated from the font size, so the layout rules still run.
export function inkOf(text: string, font: string): Ink {
  const key = `${font}|${text}`;
  let ink = inkCache.get(key);
  if (!ink) {
    if (measure === undefined) measure = document.createElement("canvas").getContext("2d");
    if (measure) {
      measure.font = font;
      const m = measure.measureText(text);
      ink = { left: -m.actualBoundingBoxLeft, right: m.actualBoundingBoxRight, top: -m.actualBoundingBoxAscent, bottom: m.actualBoundingBoxDescent, advance: m.width };
    } else {
      const px = Number(/(\d+(?:\.\d+)?)px/.exec(font)?.[1] || 14);
      const width = [...text].length * px * 0.55;
      ink = { left: 0, right: width, top: -px * 0.72, bottom: px * 0.02, advance: width };
    }
    inkCache.set(key, ink);
  }
  return ink;
}
export const barFonts = (m: BarMetrics) => ({ name: `500 ${m.name}px "Bar Roboto"`, text: `400 ${m.text}px "Bar Roboto"`, icon: `${m.icon}px "Tile Icons"` });
// Same integer arithmetic as header_bar::gaps() in the firmware, from the digit height in pixels.
export function barGaps(cap: number) {
  return { icon: Math.max(2, Math.floor((cap * 4 + 5) / 10)), item: Math.max(6, Math.floor((cap * 125 + 50) / 100)), name: Math.max(8, Math.floor((cap * 16 + 5) / 10)) };
}
export type BarPart = {
  index: number; item: HeaderItem; view: ItemView; shown: boolean; width: number; x?: number;
  dial?: number; icon?: { glyph: string; ink: Ink }; text?: { value: string; ink: Ink };
};
export type BarLayout = ReturnType<typeof barLayout>;
// The parts per item with their ink widths, the placement, and which items fall off.
export function barLayout(items: HeaderItem[], metrics: BarMetrics, nameText: string, viewOf: (item: HeaderItem) => ItemView) {
  const fonts = barFonts(metrics);
  // The firmware reads the digit height as a whole number of pixels (the glyph box of "0").
  const zero = inkOf("0", fonts.text), cap = Math.round(zero.bottom - zero.top), gaps = barGaps(cap);
  const dialInk = inkOf(String.fromCodePoint(0xf0150), fonts.icon), dial = Math.round(dialInk.bottom - dialInk.top);
  const parts: BarPart[] = items.map((item, index) => {
    const view = viewOf(item);
    const part: BarPart = { index, item, view, shown: view.shown, width: 0 };
    if (view.analog) { part.dial = dial; part.width = dial; return part; }
    if (view.icon) { part.icon = { glyph: glyph(view.icon), ink: inkOf(glyph(view.icon), fonts.icon) }; part.width += part.icon.ink.right - part.icon.ink.left; }
    if (view.text) {
      part.text = { value: view.text, ink: inkOf(view.text, fonts.text) };
      part.width += (part.icon ? gaps.icon : 0) + part.text.ink.right - part.text.ink.left;
    }
    return part;
  });
  const shown = parts.filter((p) => p.shown);
  const natural = inkOf(nameText, fonts.name).advance;
  const minName = Math.min(natural, Math.floor((metrics.width * 35) / 100));
  const total = (list: BarPart[]) => list.reduce((sum, p) => sum + p.width, 0) + Math.max(0, list.length - 1) * gaps.item;
  let first = 0;
  while (first < shown.length && total(shown.slice(first)) + gaps.name + minName > metrics.width) first++;
  const placed = shown.slice(first), dropped = new Set(shown.slice(0, first).map((p) => p.index));
  let x = metrics.width - total(placed);
  for (const p of placed) { p.x = x; x += p.width + gaps.item; }
  const nameRoom = placed.length ? placed[0].x! - gaps.name : metrics.width;
  return { metrics, fonts, cap, zero, gaps, parts, placed, dropped, nameText, natural, nameRoom };
}
// LVGL's LV_LABEL_LONG_DOT: the longest start that fits with "..." after it.
export function dotted(text: string, font: string, room: number) {
  if (inkOf(text, font).advance <= room) return text;
  const chars = [...text];
  while (chars.length && inkOf(chars.join("") + "...", font).advance > room) chars.pop();
  return chars.join("") + "...";
}
// The fonts load on first use; measurements before that are wrong, so draw again once they are in.
export function whenBarFontsLoad(then: () => void) {
  if (!("fonts" in document)) return;
  Promise.all([
    document.fonts.load('500 27px "Bar Roboto"', "Studio 0"),
    document.fonts.load('400 21px "Bar Roboto"', "Away 0"),
    document.fonts.load('26px "Tile Icons"', String.fromCodePoint(0xf0150)),
  ]).then(() => { clearInkCache(); then(); }).catch(() => {});
}
