<script setup lang="ts">
// Screen settings: the same groups and rows as the settings page on the screen itself. Every change applies at
// once, like on the screen; no Save needed.
import { computed } from "vue";
import { glyph } from "../model/topbar";
import { currentScreen, SETTING_GROUPS, setSetting, settingText, settingValues, settingsView, state, steppedSetting, type SettingRow } from "../store";

const view = computed(() => settingsView());
const values = computed(() => settingValues());
const offline = computed(() => view.value?.owner === "screen" && !currentScreen.value?.online);
const groups = computed(() => SETTING_GROUPS.map((group) => ({ ...group, rows: (group.rows as readonly SettingRow[]).filter((row) => view.value?.keys.includes(row.key)) })).filter((g) => g.rows.length));
const unavailable = (row: SettingRow) => offline.value || Boolean(view.value?.unavailable.includes(row.key));
const needs = (row: SettingRow) => (row.needs ? Boolean(values.value[row.needs]) : true);
const status = computed(() => offline.value
  ? "This screen is offline. Its settings can change once it's back."
  : state.settingPending
    ? "Saving…"
    : view.value?.owner === "screen"
      ? "Changes apply on the screen at once, and show up here when they change there."
      : "Changes apply at once. Firmware 0.2.49 lets the screen keep them itself.");
const stepDisabled = (row: SettingRow, direction: number) => {
  const value = values.value[row.key];
  if (unavailable(row) || !needs(row) || value === null || value === undefined) return true;
  return row.kind !== "moment" && steppedSetting(row, value, direction, false, values.value) === value;
};
// A key held down steps again and again, faster after a moment, like the -/+ keys on the screen.
const holds = new WeakMap<HTMLElement, { timer: number; held: boolean }>();
function step(row: SettingRow, direction: number, held: boolean) {
  const v = settingValues();
  const next = steppedSetting(row, v[row.key], direction, held, v);
  if (next !== v[row.key]) setSetting(row.key, next, 600);
}
function down(e: PointerEvent, row: SettingRow, direction: number) {
  const button = e.currentTarget as HTMLButtonElement;
  if (button.disabled || e.button !== 0) return;
  const hold = { timer: 0, held: false };
  holds.set(button, hold);
  let repeats = 0;
  hold.timer = window.setTimeout(function repeat() {
    hold.held = true;
    repeats += 1;
    step(row, direction, repeats > 5);
    hold.timer = window.setTimeout(repeat, 180);
  }, 450);
}
function up(e: PointerEvent) {
  const hold = holds.get(e.currentTarget as HTMLElement);
  if (hold) clearTimeout(hold.timer);
}
function click(e: MouseEvent, row: SettingRow, direction: number) {
  const hold = holds.get(e.currentTarget as HTMLElement);
  if (hold?.held) { hold.held = false; return; }
  step(row, direction, false);
}
</script>

<template>
  <div class="settings" id="general-settings" :class="{ offline }">
    <span class="status" id="settings-status" role="status">{{ status }}</span>
    <div v-if="view" class="set-grid" id="settings-groups">
      <section v-for="group in groups" :key="group.title" class="set-card">
        <h4><span class="mdi">{{ glyph(group.icon) }}</span>{{ group.title }}</h4>
        <div v-for="row in group.rows" :key="row.key" class="srow" :class="[`setting-${row.kind}`, { inactive: !needs(row) || unavailable(row) }]" :data-setting="row.key"
          :title="unavailable(row) && !offline ? 'This entity is off in Home Assistant, or the screen is restarting.' : ''"
          @click="row.kind === 'toggle' && ($event.target as HTMLElement).closest('.srow') === $event.currentTarget && !($event.target as HTMLElement).closest('button') && !unavailable(row) && setSetting(row.key, !values[row.key], 150)">
          <span class="s-label" :id="`setting-label-${row.key}`">{{ row.label }}</span>
          <div class="s-control">
            <button v-if="row.kind === 'toggle'" type="button" class="switch" :class="{ unknown: values[row.key] === null || values[row.key] === undefined }" role="switch"
              :id="`setting-${row.key}`" :aria-checked="Boolean(values[row.key]) ? 'true' : 'false'" :aria-labelledby="`setting-label-${row.key}`"
              :disabled="unavailable(row)" @click.stop="setSetting(row.key, !values[row.key], 150)"></button>
            <div v-else-if="row.kind === 'choice'" class="seg" role="group" :aria-labelledby="`setting-label-${row.key}`">
              <button v-for="[value, text] in row.options!" :key="String(value)" type="button" :aria-pressed="values[row.key] === value ? 'true' : 'false'" :disabled="unavailable(row)" @click="setSetting(row.key, value, 150)">{{ text }}</button>
            </div>
            <div v-else class="step">
              <button type="button" :aria-label="`${row.label} lower`" :disabled="stepDisabled(row, -1)" @pointerdown="down($event, row, -1)" @pointerup="up" @pointercancel="up" @pointerleave="up" @click="click($event, row, -1)"><span class="mdi">{{ glyph("F0374") }}</span></button>
              <output :id="`setting-${row.key}`" :aria-labelledby="`setting-label-${row.key}`">{{ settingText(row, values) }}</output>
              <button type="button" :aria-label="`${row.label} higher`" :disabled="stepDisabled(row, 1)" @pointerdown="down($event, row, 1)" @pointerup="up" @pointercancel="up" @pointerleave="up" @click="click($event, row, 1)"><span class="mdi">{{ glyph("F0415") }}</span></button>
            </div>
          </div>
        </div>
      </section>
    </div>
  </div>
</template>
