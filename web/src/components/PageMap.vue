<script setup lang="ts">
import { computed, nextTick, onBeforeUnmount, onMounted, ref, watch } from "vue";
import { t } from "../i18n";
import { connections, titleOf } from "../model/pages";
import { glyph } from "../model/topbar";
import { arrangeFromHome, connectTile, deviceStyle, liveEntries, moveWorkspacePage, openPage, openTile, screenShape, state, workspacePositions } from "../store";
import DevicePage from "./DevicePage.vue";
defineProps<{ compact?: boolean }>();
const allConnections = ref(false);

const world = ref<HTMLElement>(), zoom = ref(0.8);
const positions = computed(workspacePositions), routes = computed(() => state.document ? connections(state.document) : []);
const cardWidth = computed(() => parseFloat(deviceStyle.value['--mockup-width']));
const pitch = computed(() => ({ x: cardWidth.value + 110, y: cardWidth.value * screenShape.value.height / screenShape.value.width + 115 }));
const extent = computed(() => ({ width: (Math.max(0, ...Object.values(positions.value).map((p) => p.x)) + 1) * pitch.value.x + 80,
  height: (Math.max(0, ...Object.values(positions.value).map((p) => p.y)) + 1) * pitch.value.y + 80 }));
const name = (id: string) => { const page = state.document?.pages.find((page) => page.id === id); return page ? titleOf(state.document!, page) : ''; };
type Edge = { tileId: string; from: string; to: string; sx: number; sy: number; path: string };
const edges = ref<Edge[]>([]), pointer = ref<{ x: number; y: number } | null>(null);
const visibleEdges = computed(() => allConnections.value ? edges.value : edges.value.filter((edge) =>
  state.selectedTile ? edge.tileId === state.selectedTile.id : edge.from === state.selectedPageId || edge.to === state.selectedPageId));
const placement = ref<{ id: string; x: number; y: number; blocked: boolean } | null>(null);
function curve(sx: number, sy: number, tx: number, ty: number) {
  const bend = Math.max(50, Math.abs(tx - sx) / 2);
  return `M ${sx} ${sy} C ${sx + bend} ${sy}, ${tx - bend} ${ty}, ${tx} ${ty}`;
}
async function measure() {
  await nextTick();
  if (!world.value) return;
  const parent = world.value.getBoundingClientRect(), scale = zoom.value;
  const next = routes.value.flatMap((route) => {
    const source = world.value!.querySelector<HTMLElement>(`[data-tile-id="${route.tileId}"]`);
    const target = world.value!.querySelector<HTMLElement>(`[data-node="${route.to}"] .device`);
    if (!source || !target) return [];
    const a = source.getBoundingClientRect(), b = target.getBoundingClientRect();
    const sx = (a.right - parent.left) / scale, sy = (a.top + a.height / 2 - parent.top) / scale;
    return [{ ...route, sx, sy, path: curve(sx, sy, (b.left - parent.left) / scale - 5, (b.top + 24 - parent.top) / scale) }];
  });
  if (JSON.stringify(next) !== JSON.stringify(edges.value)) edges.value = next;
}
watch([() => state.document, positions, zoom, pitch, () => state.fontsVersion], measure, { immediate: true });
let observer: ResizeObserver | undefined;
onMounted(() => {
  if (typeof ResizeObserver !== 'undefined' && world.value) {
    observer = new ResizeObserver(measure); observer.observe(world.value);
  }
});
function selectRoute(tileId: string) { const tile = state.layout?.tiles.find((tile) => tile.id === tileId); if (tile) openTile(tile); }
function destinationClick(event: MouseEvent, id: string) {
  if (!state.connectingTileId) return;
  event.preventDefault(); event.stopPropagation();
  connectTile(state.connectingTileId, id); state.connectingTileId = null;
}
let cleanGesture = () => {};
function drag(event: PointerEvent, move: (event: PointerEvent) => void, finish: (event: PointerEvent) => void) {
  if (event.button !== 0) return;
  event.preventDefault(); cleanGesture();
  const stop = (end: PointerEvent) => { cleanGesture(); if (end.type === 'pointerup') finish(end); else state.connectingTileId = null; };
  cleanGesture = () => { window.removeEventListener('pointermove', move); window.removeEventListener('pointerup', stop); window.removeEventListener('pointercancel', stop); pointer.value = null; placement.value = null; };
  window.addEventListener('pointermove', move); window.addEventListener('pointerup', stop); window.addEventListener('pointercancel', stop);
}
function movePage(event: PointerEvent, id: string) {
  const start = { x: event.clientX, y: event.clientY }, from = positions.value[id];
  const point = (move: PointerEvent) => ({
    x: Math.min(100, Math.max(0, from.x + Math.round((move.clientX - start.x) / zoom.value / pitch.value.x))),
    y: Math.min(100, Math.max(0, from.y + Math.round((move.clientY - start.y) / zoom.value / pitch.value.y))),
  });
  drag(event, (move) => {
    const to = point(move);
    placement.value = { id, ...to, blocked: Object.entries(positions.value).some(([key, p]) => key !== id && p.x === to.x && p.y === to.y) };
  }, (end) => { const to = point(end); moveWorkspacePage(id, to.x, to.y); });
}
function beginConnection(event: PointerEvent, tileId: string) {
  state.connectingTileId = tileId;
  const start = { x: event.clientX, y: event.clientY };
  drag(event, (move) => { const rect = world.value!.getBoundingClientRect(); pointer.value = { x: (move.clientX - rect.left) / zoom.value, y: (move.clientY - rect.top) / zoom.value }; }, (end) => {
    if (Math.hypot(end.clientX - start.x, end.clientY - start.y) < 6) return;
    const id = document.elementFromPoint(end.clientX, end.clientY)?.closest<HTMLElement>('[data-node]')?.dataset.node;
    if (id) connectTile(tileId, id);
    state.connectingTileId = null;
  });
}
const pending = computed(() => {
  const edge = edges.value.find((edge) => edge.tileId === state.connectingTileId);
  return edge && pointer.value ? curve(edge.sx, edge.sy, pointer.value.x, pointer.value.y) : '';
});
onBeforeUnmount(() => { cleanGesture(); observer?.disconnect(); state.connectingTileId = null; });
</script>

<template>
  <div v-if="compact" class="map-list">
    <section v-for="(page, index) in state.document!.pages" :key="page.id">
      <div class="map-list-head"><button class="btn" @click="openPage(page.id)">{{ index + 1 }} · {{ name(page.id) }}</button>
        <button class="btn mini" @click="state.selectedPageId = page.id; state.focusedPageId = page.id">{{ t('editor.pages.edit_page') }}</button></div>
      <button v-for="route in routes.filter((route) => route.from === page.id || route.to === page.id)" :key="route.tileId" class="map-list-route" @click="selectRoute(route.tileId)">{{ name(route.from) }} → {{ name(route.to) }}</button>
    </section>
  </div>
  <template v-else>
  <div class="map-toolbar">
    <span>{{ t('editor.pages.map_hint') }}</span>
    <button class="btn mini" :aria-pressed="allConnections" @click="allConnections = !allConnections">{{ t('editor.pages.all_connections') }}</button>
    <button type="button" class="btn mini" @click="arrangeFromHome">{{ t('editor.pages.arrange') }}</button>
    <button type="button" class="icon-btn" :aria-label="t('editor.pages.zoom_out')" @click="zoom = Math.max(.4, zoom - .1)">−</button>
    <span>{{ Math.round(zoom * 100) }}%</span>
    <button type="button" class="icon-btn" :aria-label="t('editor.pages.zoom_in')" @click="zoom = Math.min(1.2, zoom + .1)">+</button>
  </div>
  <div v-if="state.connectingTileId" class="connection-instruction" role="status">
    {{ t('editor.pages.choose_destination') }}
    <button type="button" class="btn mini" @click="state.connectingTileId = null">{{ t('editor.common.cancel') }}</button>
  </div>
  <div class="map-scroll" @keydown.esc="state.connectingTileId = null">
    <div :style="{ width: `${extent.width * zoom}px`, height: `${extent.height * zoom}px` }">
      <div ref="world" class="map-world" :style="{ width: `${extent.width}px`, height: `${extent.height}px`, transform: `scale(${zoom})` }">
        <svg class="map-links" :width="extent.width" :height="extent.height" :aria-label="t('editor.pages.routes')">
          <defs><marker id="page-link-arrow" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse"><path d="M 0 0 L 10 5 L 0 10 z" /></marker></defs>
          <g v-for="edge in visibleEdges" :key="edge.tileId" :class="{ chosen: state.selectedTile?.id === edge.tileId }">
            <path class="edge" :d="edge.path" marker-end="url(#page-link-arrow)" />
            <path class="edge-hit" :d="edge.path" role="button" tabindex="0" :aria-label="`${name(edge.from)} → ${name(edge.to)}`"
              @click.stop="selectRoute(edge.tileId)" @keydown.enter.prevent="selectRoute(edge.tileId)" />
          </g>
          <path v-if="pending" class="edge pending" :d="pending" />
        </svg>
        <div v-if="placement" class="map-placement" :class="{ blocked: placement.blocked }" aria-hidden="true"
          :style="{ left: `${placement.x * pitch.x + 35}px`, top: `${placement.y * pitch.y + 35}px`, width: `${cardWidth + 10}px`, height: `${pitch.y - 35}px` }"></div>
        <article v-for="(page, index) in state.document?.pages" :key="page.id" class="map-node" :data-node="page.id"
          :class="{ selected: state.selectedPageId === page.id, destination: !!state.connectingTileId }"
          :style="{ left: `${positions[page.id].x * pitch.x + 40}px`, top: `${positions[page.id].y * pitch.y + 40}px`, width: `${cardWidth}px` }"
          @click.capture="destinationClick($event, page.id)">
          <div class="map-node-head">
            <button type="button" class="map-handle" :aria-label="t('editor.pages.move_map', { name: name(page.id) })" @pointerdown.stop="movePage($event, page.id)" @click="openPage(page.id)">
              <span class="mdi">{{ glyph('F01DB') }}</span> {{ name(page.id) }}
            </button>
            <button type="button" class="btn mini" @click="state.focusedPageId = page.id">{{ t('editor.pages.edit_page') }}</button>
          </div>
          <DevicePage :page="index" :entries="state.drag.preview || liveEntries()" :pages="state.document!.pages.length" :moving="state.drag.moving" map />
        </article>
        <button v-for="edge in visibleEdges" :key="edge.tileId" type="button" class="connection-port"
          :style="{ left: `${edge.sx - 6}px`, top: `${edge.sy - 6}px` }" :aria-label="t('editor.pages.change_destination', { name: name(edge.from) })"
          @pointerdown.stop="beginConnection($event, edge.tileId)" @keydown.enter.prevent="state.connectingTileId = edge.tileId"></button>
      </div>
    </div>
  </div>
  </template>
</template>

<style scoped>
.map-list section { border: 1px solid var(--line); border-radius: 10px; padding: 12px; margin: 12px 0; }
.map-list-head { display: flex; justify-content: space-between; gap: 8px; }
.map-list-route { display: block; margin: 10px 0; border: 0; background: none; color: var(--accent); text-align: left; }
.map-toolbar { display: flex; align-items: center; gap: 10px; font-size: 12px; margin-bottom: 14px; color: var(--muted); }
.map-toolbar > span:first-child { margin-right: auto; }
.map-scroll { overflow: auto; min-height: 500px; max-height: calc(100vh - 250px); background-image: radial-gradient(var(--line) 1px, transparent 1px); background-size: 20px 20px; border-radius: 14px; }
.map-world { position: relative; transform-origin: top left; }
.map-node { position: absolute; }
.map-node.selected :deep(.device) { outline: 2px solid var(--accent); outline-offset: 4px; }
.map-node.destination:hover :deep(.device) { outline: 3px solid var(--accent); outline-offset: 4px; }
.map-node-head { display: flex; align-items: center; gap: 6px; margin-bottom: 5px; }
.map-handle { border: 0; background: none; text-align: left; flex: 1; padding: 8px 0; font-weight: 600; cursor: grab; touch-action: none; }
.map-handle .mdi { color: var(--muted); }
.map-links { position: absolute; inset: 0; overflow: visible; pointer-events: none; }
.edge { fill: none; stroke: var(--accent); stroke-width: 1.5; opacity: .55; }
marker path { fill: var(--accent); }
.edge-hit { fill: none; stroke: transparent; stroke-width: 15; pointer-events: stroke; cursor: pointer; }
.chosen .edge, .edge-hit:focus { stroke-width: 3; opacity: 1; }
.pending { stroke-dasharray: 5 5; opacity: 1; }
.connection-port { position: absolute; width: 12px; height: 12px; border: 2px solid var(--accent); border-radius: 50%; background: var(--surface); padding: 0; touch-action: none; }
.map-placement { position: absolute; pointer-events: none; border: 3px dashed var(--accent); border-radius: 20px; background: color-mix(in srgb, var(--accent) 10%, transparent); }
.map-placement.blocked { border-color: var(--danger); }
.connection-port:focus-visible { outline: 4px solid var(--accent); outline-offset: 3px; }
.connection-instruction { display: flex; align-items: center; gap: 16px; padding: 10px; color: var(--accent); }
</style>
