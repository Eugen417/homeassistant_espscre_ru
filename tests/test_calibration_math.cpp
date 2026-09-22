#include "screen_text_en.h"
#include "../components/smart_display/calibration_math.h"
#include <cassert>

using namespace screen_calibration;

// What the whole chain from panel to glass does on one board in one orientation: the touchscreen scales the
// corrected reading, the board's transform may mirror an axis, LVGL turns the point. The wizard is never told
// any of this; it only ever sees a raw reading and the point the screen reported for it.
struct Chain {
  double ax, bx, cx;  // screen x from the corrected reading
  double ay, by, cy;  // screen y
  Point operator()(Point corrected_reading) const {
    return {ax*corrected_reading.x + bx*corrected_reading.y + cx,
            ay*corrected_reading.x + by*corrected_reading.y + cy};
  }
};

// A panel whose readings are already perfect: the taps that land exactly on the crosses through `chain`.
// `skew` bends the panel so the fit has something to correct.
static void taps(const Chain &chain, std::array<Point,5> &raw, std::array<Point,5> &reported,
                 const Calibration &current, double skew = 0.0) {
  // Invert the chain by hand for the test: solve the two equations for the reading that lands on the target.
  for (int n = 0; n < 5; ++n) {
    const Point t = target(n);
    const double det = chain.ax*chain.by - chain.bx*chain.ay;
    const double rx = ( chain.by*(t.x - chain.cx) - chain.bx*(t.y - chain.cy)) / det;
    const double ry = (-chain.ay*(t.x - chain.cx) + chain.ax*(t.y - chain.cy)) / det;
    // The panel sends something a little off, and the correction in place turns it into `rx, ry`.
    raw[n] = {rx + skew*t.y, ry - skew*t.x};
    reported[n] = chain(corrected(current, raw[n]));
  }
}

static void every_cross_comes_back(const Chain &chain, double skew = 0.0) {
  Calibration cal;  // the correction in place while the wizard runs
  std::array<Point,5> raw, reported;
  taps(chain, raw, reported, cal, skew);
  assert(fit(raw, reported, cal));
  for (int n = 0; n < 5; ++n) {
    const Point landed = project(cal, raw[n]);
    const Point t = target(n);
    assert(std::hypot(landed.x - t.x, landed.y - t.y) < 1.0);
  }
}

int main() {
  // A CYD lying down: the canvas this wizard has always run on.
  bind(320, 240);
  assert(target(0).x == 20 && target(0).y == 20);
  assert(target(1).x == 299 && target(2).y == 219);
  assert(target(4).x == 160 && target(4).y == 120);

  // The chain a CYD lying down actually has: a quarter turn, so the panel's second axis runs across the glass,
  // and the board's mirror puts the first one the other way round.
  const Chain lying{0, 320.0/4096, 0, 240.0/4096, 0, 0};
  every_cross_comes_back(lying);
  every_cross_comes_back(lying, 0.4);  // a panel whose axes lean into each other

  // The same board built standing up. This is the case that was wrong: the wizard worked the axes out from the
  // angle, and on this chain it fitted the vertical one upside down, so the top of the glass answered for the
  // bottom. Nothing is worked out any more, so the sign of every axis comes from the taps.
  bind(240, 320);
  assert(target(1).x == 219 && target(2).y == 299);
  const Chain standing{-240.0/4096, 0, 240, 0, 320.0/4096, 0};
  every_cross_comes_back(standing);
  every_cross_comes_back(standing, 0.4);

  // And the three other ways a panel can be wired, to be sure no sign is baked in anywhere.
  every_cross_comes_back(Chain{240.0/4096, 0, 0, 0, -320.0/4096, 320});
  every_cross_comes_back(Chain{-240.0/4096, 0, 240, 0, -320.0/4096, 320});
  every_cross_comes_back(Chain{0, 240.0/4096, 0, -320.0/4096, 0, 320});

  // A wizard that cannot see a straight answer refuses rather than saving a wrong one.
  bind(320, 240);
  Calibration cal;
  std::array<Point,5> raw, reported;
  taps(lying, raw, reported, cal);
  auto broken = reported;
  broken[4] = {broken[4].x + 90, broken[4].y};   // the held-out middle cross disagrees
  Calibration middle = cal;
  assert(!fit(raw, broken, middle));
  Calibration same = cal;
  for (auto &r : raw) r = {1000, 1000};          // five taps in one spot say nothing
  assert(!fit(raw, reported, same));

  // Both halves of a tap have to come from the same instant (firmware 0.2.97+). The wizard reads the panel and
  // the screen point from one filtered reading, so a pair is exact however noisy that reading is; what it may
  // never do is take the screen point at touch down and the panel's reading at lift. A finger that rolls the
  // same way on every cross then shifts every reported point together, and a shift in the reported points is a
  // shift in the chain the wizard learns, which it hands straight on to the correction.
  bind(320, 240);
  {
    const double roll = 100;  // ADC counts a rolling finger travels: about 9 px on this panel
    Calibration base;
    std::array<Point,5> tapped, matched, rolled;
    taps(lying, tapped, matched, base);
    // The pair the old code made: the reading from where the finger ended, the screen point from where it began.
    for (int n = 0; n < 5; ++n) rolled[n] = lying(corrected(base, {tapped[n].x - roll, tapped[n].y - roll}));
    // Measured against the real chain, not against the one the wizard just fitted: a wizard that mislearns the
    // chain also projects with it, so `project` would flatter exactly the mistake this is looking for.
    auto worst_on_the_glass = [&lying](const Calibration &c, const std::array<Point,5> &readings) {
      double worst = 0;
      for (int n = 0; n < 5; ++n) {
        const Point landed = lying(corrected(c, readings[n])), t = target(n);
        worst = std::max(worst, std::hypot(landed.x - t.x, landed.y - t.y));
      }
      return worst;
    };
    Calibration mismatched = base;
    // The fit's own checks pass: it holds the shifted chain against the shifted points and they agree.
    assert(fit(tapped, rolled, mismatched));
    // On the glass the shift is still there, and every tap lands beside its target by about it.
    assert(worst_on_the_glass(mismatched, tapped) > 5.0);
    // The same taps with both halves read at one instant: back on the crosses.
    Calibration paired = base;
    assert(fit(tapped, matched, paired));
    assert(worst_on_the_glass(paired, tapped) < 1.0);
  }

  // A cross is never asked for outside the glass, whichever way it hangs.
  for (auto shape : {std::array<int,2>{320,240}, {240,320}, {800,480}, {480,800}}) {
    bind(shape[0], shape[1]);
    for (int i = 0; i < 5; ++i) {
      const Point t = target(i);
      assert(t.x >= 0 && t.x < canvas.width && t.y >= 0 && t.y < canvas.height);
    }
  }
  bind(320, 240);
}
