// The sidebar's width and whether it is folded to its icons (app 0.4.85): dragged by its edge, folded with its button.
// Kept per browser, as the library's state is: a convenience of this computer, not a setting of the screens.
import { reactive, watch } from "vue";

export const SIDE_MIN = 200, SIDE_MAX = 420, SIDE_DEFAULT = 248;
// The icons alone: the board icon, the dot beside it and the room around them.
export const SIDE_FOLDED = 60;
// Dragged narrower than this, the sidebar folds to its icons; out of its icons past it, it opens again.
const FOLD_AT = 140;
const WIDTH_KEY = "esp-screens.sidebar-width", FOLDED_KEY = "esp-screens.sidebar-folded";

const read = (key: string) => { try { return localStorage.getItem(key); } catch { return null; } };
const write = (key: string, value: string) => { try { localStorage.setItem(key, value); } catch { /* private window: this visit only */ } };
export const clampWidth = (px: number) => Math.round(Math.min(SIDE_MAX, Math.max(SIDE_MIN, px)));

export const sidebar = reactive({
  width: clampWidth(Number(read(WIDTH_KEY)) || SIDE_DEFAULT),
  folded: read(FOLDED_KEY) === "1",
  resizing: false,
});
watch(() => sidebar.width, (width) => write(WIDTH_KEY, String(width)));
watch(() => sidebar.folded, (folded) => write(FOLDED_KEY, folded ? "1" : "0"));

export const sideWidth = () => (sidebar.folded ? SIDE_FOLDED : sidebar.width);
export function toggleSidebar() { sidebar.folded = !sidebar.folded; }
/** The edge dragged to `px` from the window's left: a width between the bounds, or folded below FOLD_AT. */
export function dragSidebar(px: number) {
  if (px < FOLD_AT) { sidebar.folded = true; return; }
  sidebar.folded = false;
  sidebar.width = clampWidth(px);
}
export function resetSidebar() { sidebar.folded = false; sidebar.width = SIDE_DEFAULT; }
