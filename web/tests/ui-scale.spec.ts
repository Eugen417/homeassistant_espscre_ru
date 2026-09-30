import { describe, expect, it } from "vitest";
import { pillMetrics, textEms, uiScale, widestSetpoint } from "../src/model/ui-scale";

describe("the firmware's sizes in the mockup (app 0.4.32)", () => {
  it("scales as ui::px does, from the board's density and look", () => {
    expect(uiScale({ dpi: 170, look: "standard" }).px(46)).toBe(46);
    expect(uiScale({ dpi: 143, look: "compact" }).px(34)).toBe(34);
    // A 7-inch at 133 dpi draws the standard look at 78 %: (46 x 78 + 50) / 100.
    expect(uiScale({ dpi: 133, look: "standard" }).px(46)).toBe(36);
    expect(pillMetrics({ dpi: 170, look: "standard", fonts: { watch_value: 38, sublabel_big: 21 } })).toEqual({ height: 48, inset: 4, key: 40, faces: [38, 21] });
  });
  it("measures a thermostat's -/+ by the widest temperature it can show, as tile_controls::widest_setpoint", () => {
    expect(widestSetpoint({ min_temp: 7, max_temp: 35, target_temp_step: 0.5 })).toBe("88.8°");
    expect(widestSetpoint({ min_temp: 45, max_temp: 95, target_temp_step: 1 })).toBe("88°");
    expect(widestSetpoint({ min_temp: 45, max_temp: 110, target_temp_step: 1 })).toBe("888°");
    expect(widestSetpoint({ min_temp: -20, max_temp: 30, target_temp_step: 1 })).toBe("-88°");
    expect(widestSetpoint({})).toBe("88.8°");   // 7 to 35 in halves when Home Assistant says nothing, as the firmware
    expect(textEms("88.8°")).toBeCloseTo(0.56 * 3 + 0.27 + 0.37);
  });
});
