#pragma once
#include <array>
#include <algorithm>
#include <cmath>
namespace screen_calibration {
struct Point { double x{}, y{}; };
struct Calibration { float c[6] = {1,0,0,0,1,0}; int bounds[4] = {280,3860,340,3860}; };

// The glass the wizard draws its crosses on (firmware 0.2.92+). Set from the display at boot, so the crosses
// follow a screen built standing up. The values below are a CYD lying down, which is where this wizard has
// always run.
struct Canvas { int width{320}, height{240}; };
inline Canvas canvas;
inline void bind(int width, int height) { canvas = {width, height}; }
// Four corners and the middle. The corners stand a finger's width in from the edge, because a resistive panel
// reads badly right at its rim, and the fifth is held out of the fit as an independent check.
constexpr int TARGET_INSET = 20;
inline Point target(int n) {
  const double far_x = canvas.width - 1 - TARGET_INSET, far_y = canvas.height - 1 - TARGET_INSET;
  switch (n) {
    case 0: return {TARGET_INSET, TARGET_INSET};
    case 1: return {far_x, TARGET_INSET};
    case 2: return {far_x, far_y};
    case 3: return {TARGET_INSET, far_y};
    default: return {canvas.width / 2.0, canvas.height / 2.0};
  }
}
inline bool solve(double matrix[3][4], double out[3]) {
  for (int column = 0; column < 3; ++column) {
    int pivot = column;
    for (int row = column + 1; row < 3; ++row) if (std::abs(matrix[row][column]) > std::abs(matrix[pivot][column])) pivot = row;
    for (int j = 0; j < 4; ++j) std::swap(matrix[column][j], matrix[pivot][j]);
    double scale = matrix[column][column];
    if (std::abs(scale) < 1e-8) return false;
    for (int j = 0; j < 4; ++j) matrix[column][j] /= scale;
    for (int row = 0; row < 3; ++row) if (row != column) {
      double factor = matrix[row][column];
      for (int j = 0; j < 4; ++j) matrix[row][j] -= factor * matrix[column][j];
    }
  }
  for (int i = 0; i < 3; ++i) out[i] = matrix[i][3];
  return true;
}

// A flat map between two planes: out = m[0]*in.x + m[1]*in.y + m[2], and the same for the second axis.
struct Affine { double m[6] = {1,0,0,0,1,0}; };
inline Point through(const Affine &a, Point p) {
  return {a.m[0]*p.x + a.m[1]*p.y + a.m[2], a.m[3]*p.x + a.m[4]*p.y + a.m[5]};
}
// Least squares over the taps: the map that takes every `from` closest to its `to`.
inline bool solve_affine(const std::array<Point,5> &from, const std::array<Point,5> &to, Affine &out) {
  for (int axis = 0; axis < 2; ++axis) {
    double matrix[3][4] = {};
    for (int n = 0; n < 5; ++n) {
      const double row[3] = {from[n].x, from[n].y, 1};
      const double want = axis == 0 ? to[n].x : to[n].y;
      for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) matrix[i][j] += row[i]*row[j];
        matrix[i][3] += row[i]*want;
      }
    }
    double coefficients[3];
    if (!solve(matrix, coefficients)) return false;
    for (int i = 0; i < 3; ++i) {
      out.m[axis*3+i] = coefficients[i];
      if (!std::isfinite(out.m[axis*3+i])) return false;
    }
  }
  return true;
}
// The panel reading a correction turns into, which is what the touchscreen driver then works from.
inline Point corrected(const Calibration &cal, Point raw) {
  auto c = cal.c;
  return {c[0]*raw.x + c[1]*raw.y + c[2], c[3]*raw.x + c[4]*raw.y + c[5]};
}

// Fit a new correction from five taps, each of which carries the panel's raw reading and the point the screen
// itself reported for that same touch (firmware 0.2.92+).
//
// The wizard used to work out, from the angle the board is turned by, which raw axis runs across the glass and
// which way. That is an arithmetic that has to agree with ESPHome's own, where the touchscreen scales its
// reading by the display's rotated width, the board's transform may mirror an axis and LVGL then turns the
// point once more; ESPHome's own guidance is that a calibration describes the panel in its original
// orientation. Derived by hand, it was right for the board lying down and upside down standing up.
//
// So it is not derived any more, it is measured. Every tap gives a pair: the reading the panel sent, and where
// the screen put it. Five pairs say exactly what that whole chain does, whichever way the glass hangs and
// whatever the board's transform says, and the wizard can then ask the only question that matters: which
// reading would have landed on the cross? `forward` answers from reading to screen and `back` the other way,
// and both are fitted from the taps themselves.
// Where a raw reading lands on the glass: the chain as the last fit measured it.
inline Affine measured;
inline Point project(const Calibration &cal, Point raw) { return through(measured, corrected(cal, raw)); }

inline bool fit(const std::array<Point,5> &raw, const std::array<Point,5> &reported, Calibration &cal) {
  if (cal.bounds[1] <= cal.bounds[0] || cal.bounds[3] <= cal.bounds[2]) return false;
  std::array<Point,5> was{}, targets{};
  for (int n = 0; n < 5; ++n) { was[n] = corrected(cal, raw[n]); targets[n] = target(n); }
  Affine forward, back;
  if (!solve_affine(was, reported, forward)) return false;   // a corrected reading -> where the screen put it
  if (!solve_affine(reported, was, back)) return false;      // a place on the screen -> the reading that reaches it
  // What the panel should have sent for each cross, and the correction that turns what it did send into that.
  std::array<Point,5> wanted{};
  for (int n = 0; n < 5; ++n) wanted[n] = through(back, targets[n]);
  for (int axis = 0; axis < 2; ++axis) {
    double matrix[3][4] = {};
    // The four corners fit the correction; the middle one is held back as an independent check.
    for (int n = 0; n < 4; ++n) {
      const double row[3] = {raw[n].x/4095, raw[n].y/4095, 1};
      const double want = axis == 0 ? wanted[n].x : wanted[n].y;
      for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) matrix[i][j] += row[i]*row[j];
        matrix[i][3] += row[i]*want;
      }
    }
    double coefficients[3];
    if (!solve(matrix, coefficients)) return false;
    for (int i = 0; i < 3; ++i) {
      cal.c[axis*3+i] = coefficients[i] / (i == 2 ? 1 : 4095);
      if (!std::isfinite(cal.c[axis*3+i]) || (i != 2 && std::abs(cal.c[axis*3+i]) > 4)) return false;
    }
  }
  measured = forward;
  // Every cross, the fifth included, has to come back where it was drawn.
  for (int n = 0; n < 5; ++n) {
    const Point landed = through(forward, corrected(cal, raw[n]));
    if (std::hypot(landed.x - targets[n].x, landed.y - targets[n].y) > 12) return false;
  }
  return true;
}
}
