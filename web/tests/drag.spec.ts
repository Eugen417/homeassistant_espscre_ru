// What a drag near an edge scrolls (app 0.2.78): on a phone the row of pages sideways and the page itself up and
// down, on a wide window the canvas both ways.
import { beforeEach, describe, expect, it } from "vitest";
import { dragScrollers, edgeStep, scrollsAlong } from "../src/drag";

// jsdom has no layout: give an element the sizes a browser would measure, and the overflow longhands it computes.
function sized(element: HTMLElement, size: { scrollWidth?: number; clientWidth?: number; scrollHeight?: number; clientHeight?: number }) {
  for (const [key, value] of Object.entries(size)) Object.defineProperty(element, key, { configurable: true, value });
  return element;
}
let canvas: HTMLElement, pages: HTMLElement;
beforeEach(() => {
  document.body.innerHTML = '<div class="canvas"><div class="pages"></div></div>';
  canvas = document.querySelector<HTMLElement>(".canvas")!;
  pages = document.querySelector<HTMLElement>(".pages")!;
});

describe("the scroller of a drag", () => {
  it("scrolls the canvas both ways on a wide window", () => {
    // The canvas scrolls (overflow: auto); the row of pages is wider than it but doesn't scroll itself.
    Object.assign(canvas.style, { overflowX: "auto", overflowY: "auto" });
    sized(canvas, { scrollWidth: 1800, clientWidth: 900, scrollHeight: 1200, clientHeight: 700 });
    sized(pages, { scrollWidth: 1800, clientWidth: 900 });
    const { x, y } = dragScrollers(document);
    expect(x).toBe(canvas);
    expect(y).toBe(canvas);
  });
  it("scrolls the row of pages sideways and the page up and down on a phone", () => {
    // At 960 px and narrower: the canvas is overflow: visible and the row of pages overflow-x: auto.
    Object.assign(canvas.style, { overflowX: "visible", overflowY: "visible" });
    pages.style.overflowX = "auto";
    sized(canvas, { scrollWidth: 390, clientWidth: 390, scrollHeight: 900, clientHeight: 900 });
    sized(pages, { scrollWidth: 1400, clientWidth: 358 });
    const { x, y } = dragScrollers(document);
    expect(x).toBe(pages);
    expect(y).toBe(document.scrollingElement);
  });
  it("falls back to the canvas when the row of pages fits", () => {
    pages.style.overflowX = "auto";
    sized(pages, { scrollWidth: 358, clientWidth: 358 });
    expect(dragScrollers(document).x).toBe(canvas);
    expect(scrollsAlong(null, "x")).toBe(false);
  });
});

describe("the step near an edge", () => {
  it("moves 12 px towards an edge within 60 px of it, and not in between", () => {
    expect(edgeStep(10, [0, 400])).toBe(-12);
    expect(edgeStep(200, [0, 400])).toBe(0);
    expect(edgeStep(390, [0, 400])).toBe(12);
    expect(edgeStep(120, [100, 900])).toBe(-12);
  });
});
