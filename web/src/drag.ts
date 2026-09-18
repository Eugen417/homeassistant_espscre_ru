// Pointer-based drag & drop, mouse and touch, from the library into the mockup and between
// cells. Touch starts after a short hold so the page still scrolls. While dragging, the
// mockup already shows where everything ends up; the drop confirms exactly that, and a
// drop off the grid changes nothing. A finished drag never doubles as a click.
import type { Directive } from "vue";
import { arrange, newTile } from "./model/layout";
import { markDirty, placeTile, state } from "./store";
import type { Tile } from "./types";

export type DragSource = { kind: "tile"; tile: Tile } | { kind: "entity"; id: string };
type Drag = {
  source: DragSource | null; element: HTMLElement | null; ghost: HTMLElement | null; timer: number;
  start: { x: number; y: number } | null; offset: { x: number; y: number }; pointerId: number | null;
  suppressUntil: number; last: { x: number; y: number } | null; scroller: number; target: number | null;
};
const drag: Drag = { source: null, element: null, ghost: null, timer: 0, start: null, offset: { x: 0, y: 0 }, pointerId: null, suppressUntil: 0, last: null, scroller: 0, target: null };

export const vDrag: Directive<HTMLElement, DragSource> = {
  mounted(element, binding) {
    (element as any).__dragSource = binding.value;
    element.addEventListener("pointerdown", (e: PointerEvent) => {
      const source = (element as any).__dragSource as DragSource;
      if (e.button !== 0 || (element as HTMLButtonElement).disabled || (e.target as HTMLElement).closest(".remove")) return;
      // No text selection while the mouse drags; touch keeps its default so the page can scroll.
      if (e.pointerType !== "touch") e.preventDefault();
      Object.assign(drag, { source, element, start: { x: e.clientX, y: e.clientY }, pointerId: e.pointerId });
      // A fast flick may leave the card before its first move event: keep the pointer until the drag begins.
      try { element.setPointerCapture(e.pointerId); } catch {}
      clearTimeout(drag.timer);
      if (e.pointerType === "touch") drag.timer = window.setTimeout(() => beginDrag(e), 260);
    });
    element.addEventListener("pointermove", (e: PointerEvent) => {
      if (state.drag.active || !drag.start || drag.element !== element) return;
      const distance = Math.hypot(e.clientX - drag.start.x, e.clientY - drag.start.y);
      if (e.pointerType === "touch") { if (distance > 10) { clearTimeout(drag.timer); drag.start = null; } return; }
      if (distance >= 6) beginDrag(e);
    });
    const cancel = () => { if (drag.element === element && !state.drag.active) { clearTimeout(drag.timer); drag.start = null; } };
    element.addEventListener("pointerup", cancel);
    element.addEventListener("pointercancel", cancel);
  },
  updated(element, binding) {
    (element as any).__dragSource = binding.value;
  },
};

function beginDrag(e: PointerEvent) {
  if (state.drag.active || !drag.start || !drag.source || !drag.element) return;
  state.drag.active = true;
  state.drag.moving = drag.source.kind === "tile" ? drag.source.tile : newTile(drag.source.id);
  state.drag.preview = null;
  drag.target = null;
  getSelection()?.removeAllRanges();
  const rect = drag.element.getBoundingClientRect();
  const ghost = drag.element.cloneNode(true) as HTMLElement;
  ghost.classList.add("drag-ghost");
  ghost.style.width = `${rect.width}px`;
  ghost.style.height = `${rect.height}px`;
  drag.offset = { x: e.clientX - rect.left, y: e.clientY - rect.top };
  document.body.append(ghost);
  drag.ghost = ghost;
  // The mockup re-renders while hovering, so the pointer is followed on the document, not the card.
  document.addEventListener("pointermove", moveDrag);
  document.addEventListener("pointerup", finishDrag);
  document.addEventListener("pointercancel", finishDrag);
  try { drag.element.releasePointerCapture(drag.pointerId!); } catch {}
  try { document.documentElement.setPointerCapture(drag.pointerId!); } catch {}
  document.addEventListener("touchmove", blockScroll, { passive: false });
  // Near the edges of the canvas the pages scroll along, so every page can be reached.
  drag.scroller = window.setInterval(() => {
    if (!drag.last) return;
    const canvas = document.querySelector<HTMLElement>(".canvas");
    if (!canvas) return;
    const r = canvas.getBoundingClientRect();
    const dx = drag.last.x < r.left + 60 ? -12 : drag.last.x > r.right - 60 ? 12 : 0;
    const dy = drag.last.y < r.top + 60 ? -12 : drag.last.y > r.bottom - 60 ? 12 : 0;
    if (dx || dy) { canvas.scrollBy(dx, dy); setTarget(slotAt(drag.last.x, drag.last.y)); }
  }, 16);
  setTarget(-1);
  moveDrag(e);
}
function blockScroll(e: TouchEvent) { if (state.drag.active) e.preventDefault(); }
function finishDrag(e: PointerEvent) { if (e.pointerId === drag.pointerId) endDrag(e.type === "pointerup"); }
function moveDrag(e: PointerEvent) {
  if (!drag.ghost || e.pointerId !== drag.pointerId) return;
  drag.last = { x: e.clientX, y: e.clientY };
  drag.ghost.style.transform = `translate(${e.clientX - drag.offset.x}px, ${e.clientY - drag.offset.y}px)`;
  setTarget(slotAt(e.clientX, e.clientY));
}
// The cell under the pointer: the nearest card or empty cell (the gaps between them count
// too); on a wide card the left or right half decides. -1 away from the mockup.
function slotAt(x: number, y: number) {
  let best: { cell: HTMLElement; r: DOMRect } | null = null, nearest = Infinity;
  for (const cell of document.querySelectorAll<HTMLElement>(".pages [data-slot]")) {
    const r = cell.getBoundingClientRect();
    const distance = Math.hypot(Math.max(r.left - x, 0, x - r.right), Math.max(r.top - y, 0, y - r.bottom));
    if (distance < nearest) { nearest = distance; best = { cell, r }; }
  }
  if (!best || nearest > 16) return -1;
  let slot = Number(best.cell.dataset.slot);
  if (best.cell.classList.contains("wide") && x > (best.r.left + best.r.right) / 2) slot += 1;
  return slot;
}
function setTarget(slot: number) {
  if (drag.target === slot || !state.layout || !state.drag.moving) return;
  drag.target = slot;
  // Off the grid: a tile from the grid shows where it came from; a new one shows nowhere yet.
  state.drag.preview = slot >= 0 ? arrange(state.layout.tiles, state.drag.moving, slot) : null;
}
function endDrag(drop: boolean) {
  const preview = state.drag.preview, moving = state.drag.moving;
  document.removeEventListener("pointermove", moveDrag);
  document.removeEventListener("pointerup", finishDrag);
  document.removeEventListener("pointercancel", finishDrag);
  document.removeEventListener("touchmove", blockScroll);
  try { document.documentElement.releasePointerCapture(drag.pointerId!); } catch {}
  drag.ghost?.remove();
  Object.assign(drag, { ghost: null, suppressUntil: Date.now() + 400, last: null, start: null, target: null, element: null, source: null });
  clearInterval(drag.scroller);
  state.drag.active = false;
  state.drag.preview = null;
  state.drag.moving = null;
  if (drop && preview && moving && state.layout) {
    const before = state.layout.tiles.map((t) => `${t.entity}@${t.slot}`).join();
    for (const { tile, slot } of preview) { tile.slot = slot; if (!state.layout.tiles.includes(tile)) state.layout.tiles.push(tile); }
    state.layout.tiles.sort((a, b) => a.slot - b.slot);
    if (state.layout.tiles.map((t) => `${t.entity}@${t.slot}`).join() !== before) markDirty();
    // Placing goes through the same rules as a click; a new tile also asks for its capabilities.
    if (!before.includes(`${moving.entity}@`)) placeTile(moving, moving.slot);
  }
}
window.addEventListener("click", (e) => { if (Date.now() < drag.suppressUntil) { e.stopPropagation(); e.preventDefault(); } }, true);
export const dragSuppressed = () => Date.now() < drag.suppressUntil;
