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
