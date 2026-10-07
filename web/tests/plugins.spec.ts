// Plugins (design): whether a plugin of the index fits a screen, the one rule the store page and its details share.
import { describe, expect, it } from "vitest";
import { EXAMPLE_INDEX, atLeast, fit, flashShare, headroomKb, type Plugin } from "../src/model/plugins";
import type { Screen } from "../src/types";

const plugin = (id: string) => EXAMPLE_INDEX.find((p) => p.id === id) as Plugin;
const screen = (board: string, more: Partial<Screen> = {}) =>
  ({ id: `text.${board}`, name: board, online: true, board, firmware: "0.51.0", pictures: board !== "cyd", layout: {}, ...more }) as Screen;

describe("plugins", () => {
  it("offers a plugin for one board only on that board", () => {
    expect(fit(plugin("audio"), screen("wavesharep4"))).toEqual({ ok: true });
    expect(fit(plugin("audio"), screen("guition"))).toEqual({ ok: false, reason: "board" });
  });

  it("asks for PSRAM where the plugin says it needs it", () => {
    const needsPsram = { ...plugin("ds18b20"), requires: { psram: true } };
    expect(fit(needsPsram, screen("cyd"))).toEqual({ ok: false, reason: "psram" });
    expect(fit(needsPsram, screen("guition"))).toEqual({ ok: true });
  });

  it("asks for newer firmware before anything else that can be fixed by the screen", () => {
    expect(fit(plugin("heating_schedule"), screen("guition", { firmware: "0.49.3" }))).toEqual({ ok: false, reason: "firmware" });
    expect(atLeast("0.50.0", "0.50.0")).toBe(true);
    expect(atLeast("0.100.0", "0.50.0")).toBe(true);
    expect(atLeast(undefined, "0.50.0")).toBe(false);
  });

  it("asks for a free pin where the plugin has its own wiring", () => {
    expect(fit(plugin("ds18b20"), screen("cyd"))).toEqual({ ok: true });
    expect(fit(plugin("ds18b20"), screen("guition"))).toEqual({ ok: false, reason: "pins" });
  });

  it("measures the room from the screen's own last build when the add-on reports it", () => {
    const measured = screen("hosyond40", { firmware_image: { size: 1_880_000, slot: 2_031_616 } });
    expect(headroomKb(measured)).toBe(9);
    expect(fit(plugin("heating_schedule"), measured)).toEqual({ ok: false, reason: "flash" });
    expect(fit(plugin("night_light"), measured)).toEqual({ ok: true });
  });

  it("keeps a 4 MB board under the 93 % line of its slot", () => {
    expect(headroomKb()).toBe(34);
    expect(fit(plugin("heating_schedule"), screen("cyd"))).toEqual({ ok: true });
    expect(fit({ ...plugin("heating_schedule"), flash_kb: 40 }, screen("cyd"))).toEqual({ ok: false, reason: "flash" });
    const share = flashShare(plugin("heating_schedule"), screen("cyd"))!;
    expect(share.before).toBeCloseTo(0.912, 3);
    expect(share.after).toBeCloseTo(0.9245, 3);
    expect(flashShare(plugin("heating_schedule"), screen("guition"))).toBeNull();
  });
});
