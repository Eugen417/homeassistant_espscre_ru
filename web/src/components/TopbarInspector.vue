<script setup lang="ts">
// The top bar: the name on the left; on the right up to six items: the time, an analog clock, the date, or an
// entity's state or last change. Choices apply live; the bar on every page follows.
import { computed, ref } from "vue";
import { barLayout, BUILTIN_ICONS, clockText, dateText, glyph, itemKey } from "../model/topbar";
import {
  automaticIcon, barMetrics, closeInspector, entityName, iconNamed, markDirty, moveTopbarItem, openBar, openBarAdd,
  removeTopbarItem, setSetting, settingValues, setTopbarItems, state, supports, topbarItems, topbarLabel, topbarMax, topbarView,
} from "../store";
import type { HeaderItem } from "../types";
import IconPicker from "./IconPicker.vue";
import Segmented from "./Segmented.vue";
import TopbarSvg from "./TopbarSvg.vue";

const props = defineProps<{ index: number }>();
const items = computed(() => topbarItems());
const item = computed<HeaderItem | undefined>(() => items.value[props.index]);
const lay = computed(() => {
  void state.fontsVersion; void state.now; void state.topbarPreviews;
  return barLayout(items.value, barMetrics.value, state.layout?.title || "Home", topbarView);
});
const overflow = computed(() => lay.value.dropped);
const needed = computed(() => state.inventory.header?.min_firmware || "0.2.32");
const supported = computed(() => { const [a, b, c] = needed.value.split(".").map(Number); return supports(a, b, c); });
const hint = computed(() => supported.value
  ? overflow.value.size ? "Not everything fits next to the name: the screen drops the dashed items. Remove one or choose a shorter name." : "Drag to reorder; tap an item to configure it."
  : `Sensors, the date, and the analog clock appear from firmware ${needed.value}; until that update, this screen shows the name and, if the time is in the bar, the clock.`);
const detail = (it: HeaderItem, i: number) => {
  const view = topbarView(it);
  return !view.shown ? "Hidden: not active right now" : overflow.value.has(i) ? "Doesn't fit next to the name" : view.analog ? "Dial" : view.text;
};
const iconOf = (it: HeaderItem) => {
  const view = topbarView(it);
  return view.analog || it.type !== "entity" ? iconNamed(BUILTIN_ICONS[it.type])?.cp : view.icon;
};
const justAdded = (it: HeaderItem) => state.topbarAdded?.key === itemKey(it) && Date.now() - state.topbarAdded.time < 1200;
function rename(value: string) {
  if (!state.layout) return;
  state.layout.title = value;
  markDirty();
}
function update(patch: Partial<HeaderItem>) {
  const list = [...items.value];
  list[props.index] = { ...list[props.index], ...patch };
  setTopbarItems(list);
}
const liveNote = computed(() => {
  if (!item.value) return "";
  const view = topbarView(item.value);
  return !view.shown ? "Hidden right now: not active" : overflow.value.has(props.index) ? "Doesn't currently fit next to the name" : "How it looks on the screen";
});
const clock24 = computed(() => settingValues().clock_24h !== false);
const samples = computed(() => ({ clock: clockText(clock24.value, new Date(state.now)), analog: "Small dial with the time", date: dateText(new Date(state.now)) } as Record<string, string>));

// Pointer drag between the rows, mouse and touch (touch after a short hold, so the list still scrolls). The order
// updates while dragging, the mockup follows, and a finished drag is not a click.
const drag = ref({ index: -1, active: false, moved: false });
let start: { x: number; y: number } | null = null, timer = 0, pointerId: number | null = null, suppressUntil = 0;
function down(e: PointerEvent, i: number) {
  if (e.button !== 0 || (e.target as HTMLElement).closest(".x")) return;
  if (e.pointerType !== "touch") e.preventDefault();
  drag.value = { index: i, active: false, moved: false };
  start = { x: e.clientX, y: e.clientY };
  pointerId = e.pointerId;
  clearTimeout(timer);
  if (e.pointerType === "touch") timer = window.setTimeout(begin, 260);
  document.addEventListener("pointermove", move);
  document.addEventListener("pointerup", end);
  document.addEventListener("pointercancel", end);
}
function begin() {
  drag.value.active = true;
  document.addEventListener("touchmove", block, { passive: false });
}
function block(e: TouchEvent) { if (drag.value.active) e.preventDefault(); }
function move(e: PointerEvent) {
  if (e.pointerId !== pointerId || !start) return;
  if (!drag.value.active) {
    const distance = Math.hypot(e.clientX - start.x, e.clientY - start.y);
    if (e.pointerType === "touch") { if (distance > 10) { clearTimeout(timer); start = null; } return; }
    if (distance < 6) return;
    begin();
  }
  // The target is the row whose middle the pointer passed.
  const rows = [...document.querySelectorAll<HTMLElement>(".items .item[data-index]")];
  let target = drag.value.index;
  rows.forEach((row, i) => {
    const r = row.getBoundingClientRect();
    if (i < drag.value.index && e.clientY < r.top + r.height / 2) target = Math.min(target, i);
    if (i > drag.value.index && e.clientY > r.top + r.height / 2) target = Math.max(target, i);
  });
  if (target !== drag.value.index && state.layout) {
    const list = [...items.value];
    list.splice(target, 0, ...list.splice(drag.value.index, 1));
    state.layout.header = { items: list };
    const selectedMoved = props.index === drag.value.index;
    drag.value.index = target;
    drag.value.moved = true;
    if (selectedMoved) openBar(target);
  }
}
function end(e: PointerEvent) {
  if (e.pointerId !== pointerId) return;
  clearTimeout(timer);
  document.removeEventListener("pointermove", move);
  document.removeEventListener("pointerup", end);
  document.removeEventListener("pointercancel", end);
  document.removeEventListener("touchmove", block);
  if (drag.value.active) {
    suppressUntil = Date.now() + 400;
    if (drag.value.moved) markDirty();
  }
  drag.value = { index: -1, active: false, moved: false };
  start = null; pointerId = null;
}
function pick(i: number) {
  if (Date.now() < suppressUntil) return;
  openBar(i);
}
function onKey(e: KeyboardEvent, i: number) {
  if (e.key === "Enter" || e.key === " ") { e.preventDefault(); openBar(i); return; }
  const step = ({ ArrowUp: -1, ArrowDown: 1 } as Record<string, number>)[e.key];
  if (step && moveTopbarItem(i, i + step)) { if (props.index === i) openBar(i + step); e.preventDefault(); }
}
</script>

<template>
  <div class="dr-head">
    <span class="av mdi" style="background: var(--seg); color: var(--ink-2)">{{ item ? glyph(iconOf(item) || "F0150") : glyph("F0150") }}</span>
    <span class="tx">
      <b>{{ item ? (item.type === "entity" ? entityName(item.entity!) : topbarLabel(item)) : "Top bar" }}</b>
      <small :class="{ mono: item?.type === 'entity' }">{{ item ? (item.type === "entity" ? item.entity : "From the screen itself, also works without Home Assistant") : `${items.length} of ${topbarMax()} items` }}</small>
    </span>
    <button type="button" class="icon-btn" aria-label="Close" @click="closeInspector">✕</button>
  </div>
  <div class="dr-body">
    <div class="f">
      <label class="f-label" for="title">Left: name</label>
      <input id="title" :value="state.layout?.title" maxlength="60" placeholder="For example: Living room" @input="rename(($event.target as HTMLInputElement).value)" />
    </div>
    <div class="f">
      <span class="f-label" id="topbar-caption">Right: time, sensors, or a clock <span style="text-transform: none; letter-spacing: 0; font-weight: 500"> · {{ items.length }} / {{ topbarMax() }}</span></span>
      <div class="items" id="topbar-chips" role="list" aria-labelledby="topbar-caption">
        <div v-for="(it, i) in items" :key="itemKey(it) + i" class="item" role="listitem" tabindex="0" :data-index="i"
          :class="{ selected: i === index, 'is-hidden': !topbarView(it).shown, 'is-overflow': overflow.has(i), 'just-added': justAdded(it), 'dragging-chip': drag.active && drag.index === i }"
          :aria-label="`${topbarLabel(it)}, slot ${i + 1}. Enter: configure, arrows: move`"
          @pointerdown="down($event, i)" @click="pick(i)" @keydown="onKey($event, i)">
          <span class="grip" aria-hidden="true">⋮⋮</span>
          <span class="av mdi" :style="topbarView(it).color ? { color: topbarView(it).color! } : undefined">{{ iconOf(it) ? glyph(iconOf(it)!) : "" }}</span>
          <span class="tx"><b>{{ topbarLabel(it) }}</b><small>{{ detail(it, i) }}</small></span>
          <button type="button" class="x" :aria-label="`Remove ${topbarLabel(it)} from the top bar`" @click.stop="removeTopbarItem(i)">✕</button>
        </div>
        <button type="button" class="ghost-btn" id="topbar-add" :disabled="items.length >= topbarMax()" :title="items.length >= topbarMax() ? `Maximum ${topbarMax()} items` : 'Add the time, date, analog clock, or an entity'" @click="openBarAdd">＋ Add time, date, or an entity</button>
      </div>
      <small id="topbar-hint" :class="{ warn: overflow.size > 0 && supported }">{{ hint }}</small>
    </div>
    <template v-if="item">
      <div class="live" id="topbar-live">
        <small>{{ liveNote }}</small>
        <TopbarSvg :items="[item]" single />
      </div>
      <template v-if="item.type === 'entity'">
        <div class="f">
          <span class="f-label">What to show</span>
          <Segmented :choices="(state.inventory.header?.contents || []).map((c) => [c.key, c.label] as [string, string])" :value="item.content" @pick="(v) => update({ content: v })" />
          <small>Last changed keeps counting on the screen itself: “Just now”, “5 min ago”, “Yesterday”.</small>
        </div>
        <IconPicker :selected="item.icon || 'auto'" :automatic="state.topbarPreviews[itemKey(item)]?.auto_icon || automaticIcon(item.entity!)" auto-label="Automatic (like Home Assistant)" allow-none @pick="(n) => update({ icon: n })" />
        <div class="f">
          <span class="f-label">Show</span>
          <Segmented :choices="(state.inventory.header?.shows || []).map((s) => [s.key, s.label] as [string, string])" :value="item.show" @pick="(v) => update({ show: v })" />
          <small>Only when active hides the item as long as it's off, closed, away, or 0. Handy for an open door, a running washing machine, or who's home.</small>
        </div>
      </template>
      <div v-else-if="item.type !== 'date'" class="f">
        <span class="f-label">Format</span>
        <Segmented :choices="[['24', '24 hour'], ['12', '12 hour']]" :value="clock24 ? '24' : '12'" @pick="(v) => setSetting('clock_24h', v === '24', 150)" />
        <small>The screen's own clock setting: it applies at once to every clock on this screen, including the clock tiles.</small>
      </div>
      <small v-else>{{ samples.date }} today. The date follows the screen's own clock.</small>
    </template>
  </div>
  <div v-if="item" class="dr-foot">
    <button type="button" class="btn danger" @click="removeTopbarItem(index)">Remove</button>
    <span class="spacer"></span>
    <button type="button" class="btn quiet" :disabled="index === 0" @click="moveTopbarItem(index, index - 1) && openBar(index - 1)">↑ Up</button>
    <button type="button" class="btn quiet" :disabled="index >= items.length - 1" @click="moveTopbarItem(index, index + 1) && openBar(index + 1)">↓ Down</button>
  </div>
</template>
