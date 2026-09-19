<script setup lang="ts">
// Screen settings: the same groups and rows as the settings page on the screen itself. Every change applies at
// once, like on the screen; no Save needed.
import { computed } from "vue";
import { t } from "../i18n";
import { glyph } from "../model/topbar";
import {
  choiceText, currentScreen, pageReachWarning, SETTING_GROUPS, setSetting, settingLabel, settingText, settingValues, settingsView, state, steppedSetting,
  type SettingRow,
} from "../store";

const view = computed(() => settingsView());
const values = computed(() => settingValues());
const offline = computed(() => view.value?.owner === "screen" && !currentScreen.value?.online);
const groups = computed(() => SETTING_GROUPS.map((group) => ({ ...group, rows: (group.rows as readonly SettingRow[]).filter((row) => view.value?.keys.includes(row.key)) })).filter((g) => g.rows.length));
// Page buttons and swiping off: the pages that only Go to page tiles could reach, and don't.
const reachWarning = computed(() => pageReachWarning());
const unavailable = (row: SettingRow) => offline.value || Boolean(view.value?.unavailable.includes(row.key));
const needs = (row: SettingRow) => (row.needs ? Boolean(values.value[row.needs]) : true);
const status = computed(() => offline.value
  ? t("editor.screen_settings.status.offline")
  : state.settingPending
    ? t("editor.common.saving")
    : view.value?.owner === "screen"
      ? t("editor.screen_settings.status.screen")
      : t("editor.screen_settings.status.app"));
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
      <section v-for="group in groups" :key="group.group" class="set-card">
        <h4><span class="mdi">{{ glyph(group.icon) }}</span>{{ t(`editor.screen_settings.groups.${group.group}`) }}</h4>
        <div v-for="row in group.rows" :key="row.key" class="srow" :class="[`setting-${row.kind}`, { inactive: !needs(row) || unavailable(row) }]" :data-setting="row.key"
          :title="unavailable(row) && !offline ? t('editor.screen_settings.unavailable') : ''"
          @click="row.kind === 'toggle' && ($event.target as HTMLElement).closest('.srow') === $event.currentTarget && !($event.target as HTMLElement).closest('button') && !unavailable(row) && setSetting(row.key, !values[row.key], 150)">
          <span class="s-label" :id="`setting-label-${row.key}`">{{ settingLabel(row) }}</span>
          <div class="s-control">
            <button v-if="row.kind === 'toggle'" type="button" class="switch" :class="{ unknown: values[row.key] === null || values[row.key] === undefined }" role="switch"
              :id="`setting-${row.key}`" :aria-checked="Boolean(values[row.key]) ? 'true' : 'false'" :aria-labelledby="`setting-label-${row.key}`"
              :disabled="unavailable(row)" @click.stop="setSetting(row.key, !values[row.key], 150)"></button>
            <div v-else-if="row.kind === 'choice'" class="seg" role="group" :aria-labelledby="`setting-label-${row.key}`">
              <button v-for="value in row.options!" :key="String(value)" type="button" :aria-pressed="values[row.key] === value ? 'true' : 'false'" :disabled="unavailable(row)" @click="setSetting(row.key, value, 150)">{{ choiceText(row, value) }}</button>
            </div>
            <div v-else class="step">
              <button type="button" :aria-label="t('editor.screen_settings.lower', { name: settingLabel(row) })" :disabled="stepDisabled(row, -1)" @pointerdown="down($event, row, -1)" @pointerup="up" @pointercancel="up" @pointerleave="up" @click="click($event, row, -1)"><span class="mdi">{{ glyph("F0374") }}</span></button>
              <output :id="`setting-${row.key}`" :aria-labelledby="`setting-label-${row.key}`">{{ settingText(row, values) }}</output>
              <button type="button" :aria-label="t('editor.screen_settings.higher', { name: settingLabel(row) })" :disabled="stepDisabled(row, 1)" @pointerdown="down($event, row, 1)" @pointerup="up" @pointercancel="up" @pointerleave="up" @click="click($event, row, 1)"><span class="mdi">{{ glyph("F0415") }}</span></button>
            </div>
          </div>
        </div>
        <p v-if="reachWarning && group.rows.some((row) => row.key === 'page_buttons')" class="hint warn" id="settings-page-reach">{{ reachWarning }}</p>
      </section>
    </div>
  </div>
</template>
