<script setup lang="ts">
// The screensaver (app 0.4.48): what this screen shows in standby instead of its dimmed tiles. A player and a camera of
// your choice and the clock, in the order the screen tries them: it shows the first one that is there right now (a
// player that plays with a cover, a camera Home Assistant has, the clock always). The add-on decides and tells the
// screen; nothing on the screensaver takes a tap, so the first touch wakes the screen as standby always did.
import { computed, ref } from "vue";
import { t } from "../i18n";
import { glyph } from "../model/topbar";
import { currentScreen, setScreensaver, settingValues, state } from "../store";
import type { SaverKind } from "../types";
import Icon from "./ui/Icon.vue";
import UiSelect from "./ui/UiSelect.vue";

const saver = computed(() => currentScreen.value?.screensaver);
const MIN_FIRMWARE = "0.29.0";
const ICONS: Record<SaverKind, string> = { media: "F075A", camera: "F07AE", clock: "F0150" };
const DOMAINS: Record<"media" | "camera", string[]> = { media: ["media_player"], camera: ["camera", "image"] };
const standbyOn = computed(() => settingValues().standby_enabled !== false && settingValues().standby_enabled !== 0);
const ready = computed(() => Boolean(saver.value?.ready));
// A board without pictures shows the clock alone, so its list has nothing to order.
const dragged = ref<SaverKind[] | null>(null);
const order = computed<SaverKind[]>(() => {
  const list = dragged.value || saver.value?.order || ["media", "camera", "clock"];
  return saver.value?.pictures ? list : list.filter((kind) => kind === "clock");
});
const isOn = (kind: SaverKind) => !saver.value?.off.includes(kind);
const choices = (kind: "media" | "camera") => [
  ["", t(`editor.screen_settings.screensaver.choose_${kind}`)] as const,
  ...state.inventory.entities
    .filter((e) => DOMAINS[kind].includes(e.id.split(".")[0]))
    .map((e) => [e.id, e.area ? `${e.name} · ${e.area}` : e.name] as const)
    .sort((a, b) => a[1].localeCompare(b[1])),
];
const detail = (kind: SaverKind) => {
  if (kind !== "clock" && !saver.value?.[kind]) return t(`editor.screen_settings.screensaver.details.${kind}_unset`);
  return t(`editor.screen_settings.screensaver.details.${kind}`);
};
const label = (kind: SaverKind) => t(`editor.screen_settings.screensaver.kinds.${kind}`);
function change(patch: Parameters<typeof setScreensaver>[1]) {
  if (currentScreen.value) setScreensaver(currentScreen.value, patch);
}
function toggle(kind: SaverKind) {
  const off = saver.value?.off || [];
  change({ off: off.includes(kind) ? off.filter((k) => k !== kind) : [...off, kind] });
}
function move(from: number, to: number) {
  const list = [...(saver.value?.order || [])];
  if (to < 0 || to >= list.length) return false;
  list.splice(to, 0, ...list.splice(from, 1));
  change({ order: list });
  return true;
}

// Pointer drag between the rows, mouse and touch (touch after a short hold, so the panel still scrolls), as the top
// bar's list does. A finger on a control in the row is no drag.
const drag = ref({ index: -1, active: false });
let start: { x: number; y: number } | null = null, timer = 0, pointerId: number | null = null;
function down(e: PointerEvent, i: number) {
  if (e.button !== 0 || !ready.value || (e.target as HTMLElement).closest("button, .ui-select")) return;
  if (e.pointerType !== "touch") e.preventDefault();
  drag.value = { index: i, active: false };
  start = { x: e.clientX, y: e.clientY };
  pointerId = e.pointerId;
  clearTimeout(timer);
  if (e.pointerType === "touch") timer = window.setTimeout(begin, 260);
  document.addEventListener("pointermove", track);
  document.addEventListener("pointerup", end);
  document.addEventListener("pointercancel", end);
}
function begin() {
  drag.value.active = true;
  dragged.value = [...(saver.value?.order || [])];
  document.addEventListener("touchmove", block, { passive: false });
}
function block(e: TouchEvent) { if (drag.value.active) e.preventDefault(); }
function track(e: PointerEvent) {
  if (e.pointerId !== pointerId || !start) return;
  if (!drag.value.active) {
    const distance = Math.hypot(e.clientX - start.x, e.clientY - start.y);
    if (e.pointerType === "touch") { if (distance > 10) { clearTimeout(timer); start = null; } return; }
    if (distance < 6) return;
    begin();
  }
  const rows = [...document.querySelectorAll<HTMLElement>("#screensaver-steps .item[data-index]")];
  let target = drag.value.index;
  rows.forEach((row, i) => {
    const r = row.getBoundingClientRect();
    if (i < drag.value.index && e.clientY < r.top + r.height / 2) target = Math.min(target, i);
    if (i > drag.value.index && e.clientY > r.top + r.height / 2) target = Math.max(target, i);
  });
  if (target !== drag.value.index && dragged.value) {
    const list = [...dragged.value];
    list.splice(target, 0, ...list.splice(drag.value.index, 1));
    dragged.value = list;
    drag.value.index = target;
  }
}
function end(e: PointerEvent) {
  if (e.pointerId !== pointerId) return;
  clearTimeout(timer);
  document.removeEventListener("pointermove", track);
  document.removeEventListener("pointerup", end);
  document.removeEventListener("pointercancel", end);
  document.removeEventListener("touchmove", block);
  if (drag.value.active && e.type !== "pointercancel" && dragged.value && dragged.value.join() !== saver.value?.order.join()) change({ order: dragged.value });
  dragged.value = null;
  drag.value = { index: -1, active: false };
  start = null; pointerId = null;
}
function onKey(e: KeyboardEvent, i: number) {
  const step = ({ ArrowUp: -1, ArrowDown: 1 } as Record<string, number>)[e.key];
  if (step && move(i, i + step)) {
    e.preventDefault();
    requestAnimationFrame(() => document.querySelector<HTMLElement>(`#screensaver-steps .item[data-index="${i + step}"]`)?.focus());
  }
}
</script>

<template>
  <section v-if="saver && saver.standby" class="set-card" id="settings-screensaver" :class="{ inactive: !ready }">
    <h4><span class="mdi">{{ glyph("F04B2") }}</span>{{ t("editor.screen_settings.screensaver.title") }}</h4>
    <div class="srow setting-toggle" :class="{ inactive: !ready || !standbyOn }" data-setting="screensaver"
      @click="ready && !($event.target as HTMLElement).closest('button') && change({ show: !saver.show })">
      <span class="s-label" id="screensaver-show-label">{{ t("editor.screen_settings.screensaver.show") }}</span>
      <div class="s-control">
        <button type="button" class="switch" role="switch" id="screensaver-show" :aria-checked="saver.show ? 'true' : 'false'"
          aria-labelledby="screensaver-show-label" :disabled="!ready" @click.stop="change({ show: !saver.show })"></button>
      </div>
    </div>
    <p v-if="!ready" class="hint" id="screensaver-firmware">{{ t("editor.screen_settings.screensaver.needs_firmware", { version: MIN_FIRMWARE }) }}</p>
    <p v-else-if="!standbyOn" class="hint">{{ t("editor.screen_settings.screensaver.standby_off") }}</p>
    <template v-if="ready && saver.show">
      <p class="hint saver-lead">{{ t(saver.pictures ? "editor.screen_settings.screensaver.order" : "editor.screen_settings.screensaver.no_pictures") }}</p>
      <div class="items" id="screensaver-steps" role="list" :aria-label="t('editor.screen_settings.screensaver.order')">
        <div v-for="(kind, i) in order" :key="kind" class="item saver-step" role="listitem" :tabindex="saver.pictures ? 0 : -1" :data-index="i" :data-kind="kind"
          :class="{ 'is-hidden': !isOn(kind), 'dragging-chip': drag.active && drag.index === i, still: !saver.pictures }"
          @pointerdown="saver.pictures && down($event, i)" @keydown="saver.pictures && onKey($event, i)">
          <Icon v-if="saver.pictures" name="drag-vertical" class="grip" />
          <span class="av mdi">{{ glyph(ICONS[kind]) }}</span>
          <span class="tx">
            <b>{{ label(kind) }}</b>
            <UiSelect v-if="kind !== 'clock'" class="saver-pick" :id="`screensaver-${kind}`" :model-value="saver[kind]" :options="choices(kind)"
              @update:model-value="(value: string) => change({ [kind]: value })" />
            <small>{{ detail(kind) }}</small>
          </span>
          <button type="button" class="switch" role="switch" :aria-checked="isOn(kind) ? 'true' : 'false'"
            :aria-label="t('editor.screen_settings.screensaver.use', { name: label(kind) })" @click.stop="toggle(kind)"></button>
        </div>
      </div>
      <p class="hint">{{ t("editor.screen_settings.screensaver.tap") }}</p>
    </template>
  </section>
</template>

<style scoped>
.set-card.inactive h4 { opacity: 0.6; }
.saver-lead { margin: 2px 0 8px; }
#screensaver-steps { margin-bottom: 8px; }
.saver-step { cursor: grab; align-items: flex-start; }
.saver-step.still { cursor: default; }
.saver-step .av, .saver-step .grip, .saver-step .switch { margin-top: 3px; }
.saver-step .tx { gap: 4px; flex: 1; }
.saver-step .tx small { white-space: normal; }
.saver-step :deep(.saver-pick) { width: 100%; }
</style>
