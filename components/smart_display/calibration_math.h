#pragma once
#include <array>
#include <algorithm>
#include <cmath>
namespace screen_calibration {
struct Point { double x{}, y{}; };
struct Calibration { float c[6] = {1,0,0,0,1,0}; int bounds[4] = {280,3860,340,3860}; };
constexpr Point TARGETS[5] = {{20,20}, {299,20}, {299,219}, {20,219}, {160,120}};
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
inline Point project(const Calibration &cal, Point raw) {
  auto c = cal.c; auto b = cal.bounds;
  return {(c[3]*raw.x+c[4]*raw.y+c[5]-b[2])*320/(b[3]-b[2]),
          (c[0]*raw.x+c[1]*raw.y+c[2]-b[0])*240/(b[1]-b[0])};
}
inline bool fit(const std::array<Point,5> &raw, Calibration &cal) {
  if (cal.bounds[1] <= cal.bounds[0] || cal.bounds[3] <= cal.bounds[2]) return false;
  for (int axis = 0; axis < 2; ++axis) {
    double matrix[3][4] = {};
    for (int n = 0; n < 4; ++n) {
      double row[3] = {raw[n].x/4095, raw[n].y/4095, 1};
      double target = axis == 0 ? cal.bounds[0]+TARGETS[n].y*(cal.bounds[1]-cal.bounds[0])/240
                               : cal.bounds[2]+TARGETS[n].x*(cal.bounds[3]-cal.bounds[2])/320;
      for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) matrix[i][j] += row[i]*row[j];
        matrix[i][3] += row[i]*target;
      }
    }
    double coefficients[3];
    if (!solve(matrix, coefficients)) return false;
    for (int i = 0; i < 3; ++i) {
      cal.c[axis*3+i] = coefficients[i] / (i == 2 ? 1 : 4095);
      if (!std::isfinite(cal.c[axis*3+i]) || (i != 2 && std::abs(cal.c[axis*3+i]) > 4)) return false;
    }
  }
  // Fifth point is held out of the fit: it independently checks the result.
  for (int n = 0; n < 5; ++n) {
    Point p = project(cal, raw[n]);
    if (std::hypot(p.x-TARGETS[n].x,p.y-TARGETS[n].y) > 12) return false;
  }
  return true;
}
}
