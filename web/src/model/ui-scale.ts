// The firmware's sizes in the editor's mockup (app 0.4.32): ui_scale.h's px() and the -/+ pill of a card's controls
// (runtime_tiles panel_metrics, stepper_keys), computed the same way from the board's density and look, so the mockup
// draws them at the size the glass does instead of sizes of its own.
type Shape = { dpi?: number; look?: string; fonts?: { watch_value?: number; sublabel_big?: number; sublabel?: number } };

/** ui::configure and ui::px: a size of the reference look (170 dpi standard, 143 dpi compact) in this board's pixels. */
export function uiScale(shape: Shape) {
  const compact = shape.look === "compact", reference = compact ? 143 : 170;
  const dpi = Math.round(shape.dpi || reference);
  const scalePct = Math.floor((dpi * 100 + Math.floor(reference / 2)) / reference);
  const px = (n: number) => (scalePct === 100 ? n : n >= 0 ? Math.floor((n * scalePct + 50) / 100) : -Math.floor((-n * scalePct + 50) / 100));
  return { compact, large: !compact, px };
}

/** The pill of a wide card's -/+ in glass pixels: its height (panel_metrics key_h + 2), the inset of its keys and their
 * diameter (stepper_keys), and the faces its number is drawn in (watch_value first, then sublabel_big, then sublabel). */
export function pillMetrics(shape: Shape) {
  const { large, px } = uiScale(shape);
  const height = px(large ? 46 : 34) + 2, inset = Math.max(2, px(large ? 4 : 3)), key = Math.max(1, height - 2 * inset);
  const fonts = shape.fonts || {};
  return { height, inset, key, faces: [fonts.watch_value, fonts.sublabel_big, fonts.sublabel].filter((size): size is number => Boolean(size)) };
}

/** A number's width in em as the screens' Roboto draws it: a digit .56, the degree sign .37, a minus .33, a decimal mark
 * .27. Enough to pick among a board's faces as the firmware does, which measures the glyphs themselves. */
export const textEms = (text: string) => [...text].reduce((sum, ch) => sum + (/\d/.test(ch) ? 0.56 : ch === "°" ? 0.37 : ch === "-" ? 0.33 : 0.27), 0);

/** The widest temperature a thermostat's -/+ can show (tile_controls::widest_setpoint): as many 8s as its highest or
 * lowest temperature has digits, with the decimal its step shows. The face measured by it keeps its size from tap to tap. */
export function widestSetpoint(a: Record<string, any>) {
  const minimum = Number.isFinite(Number(a.min_temp)) ? Number(a.min_temp) : 7, maximum = Number.isFinite(Number(a.max_temp)) ? Number(a.max_temp) : 35;
  const reach = Math.max(Math.abs(minimum), Math.abs(maximum)), step = Number(a.target_temp_step) > 0 ? Number(a.target_temp_step) : 0.5;
  return `${minimum < 0 ? "-" : ""}${"8".repeat(String(Math.trunc(reach)).length)}${step < 1 ? ".8" : ""}°`;
}
