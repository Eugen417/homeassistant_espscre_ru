// The sidebar's width and fold (app 0.4.85): bounded, folded when dragged past the narrowest, kept per browser.
import { describe, expect, it } from "vitest";
import { nextTick } from "vue";
import { SIDE_DEFAULT, SIDE_FOLDED, SIDE_MAX, SIDE_MIN, dragSidebar, resetSidebar, sideWidth, sidebar, toggleSidebar } from "../src/sidebar-state";

describe("the sidebar's edge", () => {
  it("keeps its width within bounds, folds past the narrowest and opens again when dragged out", async () => {
    resetSidebar();
    dragSidebar(1000);
    expect(sideWidth()).toBe(SIDE_MAX);
    dragSidebar(150);
    expect([sidebar.folded, sideWidth()]).toEqual([false, SIDE_MIN]);
    dragSidebar(90);
    expect([sidebar.folded, sideWidth()]).toEqual([true, SIDE_FOLDED]);
    dragSidebar(300);
    expect([sidebar.folded, sideWidth()]).toEqual([false, 300]);
    await nextTick();
    expect(localStorage.getItem("esp-screens.sidebar-width")).toBe("300");
  });
  it("folds and unfolds with its button, keeping the width it had", async () => {
    dragSidebar(320);
    toggleSidebar();
    expect(sideWidth()).toBe(SIDE_FOLDED);
    await nextTick();
    expect(localStorage.getItem("esp-screens.sidebar-folded")).toBe("1");
    toggleSidebar();
    expect(sideWidth()).toBe(320);
    resetSidebar();
    expect([sidebar.folded, sideWidth()]).toEqual([false, SIDE_DEFAULT]);
  });
});
