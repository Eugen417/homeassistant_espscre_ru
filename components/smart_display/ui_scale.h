#pragma once
// LAB (responsive): one scale for every pixel size the C++ still carries as a number. The board sets it at boot
// (UI_SCALE_PCT: 100 on the CYD and the Guition, dpi / 170 * 100 on a board of the standard look). The real
// round replaces this with lv_dpx() and one Scale; this keeps the two existing boards bit-identical meanwhile.
namespace ui {
inline int scale_pct = 100;
inline int px(int n) {
  if (scale_pct == 100) return n;
  return n >= 0 ? (n * scale_pct + 50) / 100 : -((-n * scale_pct + 50) / 100);
}
}  // namespace ui
