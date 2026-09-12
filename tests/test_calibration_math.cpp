#include "../components/smart_display/calibration_math.h"
#include <cassert>
int main() {
  using namespace screen_calibration;
  Calibration c;
  std::array<Point,5> raw;
  for (int i=0;i<5;++i) raw[i] = {280+TARGETS[i].y*3580/240,340+TARGETS[i].x*3520/320};
  assert(fit(raw,c));
  for(int i=0;i<5;++i) { auto p=project(c,raw[i]); assert(std::hypot(p.x-TARGETS[i].x,p.y-TARGETS[i].y)<0.01); }
  raw[4].x+=600; assert(!fit(raw,c)); // independent center check
  for (auto &r:raw) r={1000,1000}; assert(!fit(raw,c));
  for (int i=0;i<5;++i) raw[i] = {280+TARGETS[i].y*3580/240+TARGETS[i].x*0.2,340+TARGETS[i].x*3520/320};
  assert(fit(raw,c)); // skew correction
}
