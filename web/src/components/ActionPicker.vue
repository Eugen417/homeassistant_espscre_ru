<script setup lang="ts">
// Perform action (app 0.2.67): the actions Home Assistant offers for a tile's entity, under its own names,
// and one field per value Home Assistant asks for (its selector). An empty field is left out.
import { computed } from "vue";
import { loadEntityActions, markDirty, state, supports } from "../store";
import type { EntityAction, Tile } from "../types";
import Segmented from "./Segmented.vue";

const props = defineProps<{ tile: Tile }>();
const list = computed(() => state.entityActions[props.tile.entity]);
const chosen = computed(() => props.tile.options?.action);
const entry = computed(() => list.value?.find((a) => a.action === chosen.value?.action));
const open = computed(() => state.actionPickerOpen || !chosen.value);
const rows = computed(() => {
  const q = state.actionSearch.trim().toLocaleLowerCase();
  return (list.value || []).filter((a) => `${a.name} ${a.action} ${a.description}`.toLocaleLowerCase().includes(q));
});
if (list.value === undefined) loadEntityActions(props.tile.entity);

function pick(action: EntityAction) {
  const same = action.action === chosen.value?.action;
  props.tile.options = { ...props.tile.options, action: same ? chosen.value : { action: action.action } };
  state.actionPickerOpen = false;
  markDirty();
}
function setField(key: string, value: unknown) {
  const data: Record<string, unknown> = { ...(props.tile.options?.action?.data || {}) };
  if (value === undefined) delete data[key];
  else data[key] = value;
  props.tile.options = { ...props.tile.options, action: { action: chosen.value!.action, ...(Object.keys(data).length ? { data } : {}) } };
  markDirty();
}
const missing = computed(() => {
  if (!entry.value) return [];
  const data = props.tile.options?.action?.data || {};
  return entry.value.fields.filter((f) => f.required && data[f.key] === undefined).map((f) => f.name);
});
type Field = EntityAction["fields"][number];
const kindOf = (field: Field) => Object.keys(field.selector || {})[0] || "text";
const configOf = (field: Field) => (field.selector || {})[kindOf(field)] || {};
const exampleOf = (field: Field) => (field.example === undefined ? "" : typeof field.example === "string" ? field.example : JSON.stringify(field.example));
// A select's own options, or the values this entity has where Home Assistant asks for one of them (a source, an effect).
function choicesOf(field: Field): [string, string][] | null {
  const config = configOf(field), kind = kindOf(field);
  if (Array.isArray(field.options)) return field.options.map((o) => [o, o]);
  if (kind === "select" && Array.isArray(config.options) && !config.multiple)
    return config.options.map((o: any) => (o && typeof o === "object" ? [String(o.value), String(o.label ?? o.value)] : [String(o), String(o)]));
  return null;
}
const hex = (rgb: number[]) => "#" + rgb.map((c) => Math.max(0, Math.min(255, Number(c) || 0)).toString(16).padStart(2, "0")).join("");
const colorValue = (value: unknown) => (Array.isArray(value) && value.length === 3 ? hex(value as number[]) : "#ffffff");
const textValue = (value: unknown) => (value === undefined ? "" : typeof value === "string" ? value : JSON.stringify(value));
function onText(field: Field, raw: string) {
  const text = raw.trim();
  if (!text) return setField(field.key, undefined);
  if (kindOf(field) === "text") return setField(field.key, raw);
  try { setField(field.key, JSON.parse(text)); } catch { setField(field.key, raw); }
}
function onNumber(field: Field, raw: string) {
  setField(field.key, raw === "" || !Number.isFinite(Number(raw)) ? undefined : Number(raw));
}
const unitOf = (field: Field) => configOf(field).unit_of_measurement || (kindOf(field) === "color_temp" ? configOf(field).unit || "" : "");
</script>

<template>
  <div class="f">
    <span class="f-label">Action</span>
    <small v-if="list === undefined">Asking Home Assistant which actions this entity has…</small>
    <small v-else-if="list === null" class="warn">Home Assistant isn't answering right now. Try again in a moment.</small>
    <template v-else>
      <button type="button" class="row" :aria-expanded="open ? 'true' : 'false'" @click="state.actionPickerOpen = !open">
        <span class="tx">
          <b>{{ entry ? entry.name : chosen ? chosen.action : "Choose an action" }}</b>
          <small v-if="chosen" class="mono">{{ chosen.action }}</small>
        </span>
        <span class="link">{{ open ? "Close" : "Change" }}</span>
      </button>
      <div v-if="open" class="picker">
        <input v-model="state.actionSearch" type="search" placeholder="Search, for example toggle, position, or play" aria-label="Search for an action" />
        <div class="action-list">
          <button v-for="action in rows" :key="action.action" type="button" class="action-choice" :aria-pressed="action.action === chosen?.action ? 'true' : 'false'" @click="pick(action)">
            <strong>{{ action.name }}</strong>
            <small>{{ action.description ? `${action.action} · ${action.description}` : action.action }}</small>
          </button>
          <p v-if="!rows.length" class="hint">No action found.</p>
        </div>
      </div>
      <small v-if="chosen && !entry" class="warn">Home Assistant doesn't offer this action for this entity any more, so a tap does nothing. Choose another one.</small>
      <template v-if="entry && !open">
        <div v-for="field in entry.fields" :key="field.key" class="f">
          <span class="f-label">{{ field.required ? field.name : `${field.name} (optional)` }}</span>
          <Segmented v-if="kindOf(field) === 'boolean'" :choices="[['', 'Not set'], ['true', 'On'], ['false', 'Off']]" :value="chosen?.data?.[field.key] === undefined ? '' : String(chosen?.data?.[field.key])"
            @pick="(v) => setField(field.key, v === '' ? undefined : v === 'true')" />
          <Segmented v-else-if="choicesOf(field) && choicesOf(field)!.length <= 4" :choices="[['', 'Not set'], ...choicesOf(field)!]" :value="chosen?.data?.[field.key] === undefined ? '' : String(chosen?.data?.[field.key])"
            @pick="(v) => setField(field.key, v === '' ? undefined : v)" />
          <select v-else-if="choicesOf(field)" :aria-label="field.name" :value="String(chosen?.data?.[field.key] ?? '')" @change="setField(field.key, ($event.target as HTMLSelectElement).value === '' ? undefined : ($event.target as HTMLSelectElement).value)">
            <option value="">Not set</option>
            <option v-for="[key, text] in choicesOf(field)!" :key="key" :value="key">{{ text }}</option>
          </select>
          <div v-else-if="kindOf(field) === 'color_rgb'" class="action-number">
            <input type="color" :aria-label="field.name" :value="colorValue(chosen?.data?.[field.key])" @input="setField(field.key, [1, 3, 5].map((i) => parseInt(($event.target as HTMLInputElement).value.slice(i, i + 2), 16)))" />
            <button type="button" class="btn quiet mini" @click="setField(field.key, undefined)">Not set</button>
          </div>
          <div v-else-if="kindOf(field) === 'number' || kindOf(field) === 'color_temp'" class="action-number">
            <input type="number" :aria-label="field.name" :min="configOf(field).min" :max="configOf(field).max" :step="configOf(field).step === 'any' ? 'any' : configOf(field).step"
              :value="chosen?.data?.[field.key] ?? ''" :placeholder="exampleOf(field)" @input="onNumber(field, ($event.target as HTMLInputElement).value)" />
            <span v-if="unitOf(field)">{{ unitOf(field) }}</span>
          </div>
          <input v-else type="text" :aria-label="field.name" :value="textValue(chosen?.data?.[field.key])" :placeholder="exampleOf(field)" @input="onText(field, ($event.target as HTMLInputElement).value)" />
          <small v-if="field.description">{{ field.description }}</small>
        </div>
        <small v-if="missing.length" class="warn">Home Assistant needs {{ missing.join(", ") }}.</small>
      </template>
      <small v-if="!supports(0, 2, 58)">The screen performs an action from firmware 0.2.58: press Update on the screen. Until then a tap works as Automatic.</small>
    </template>
  </div>
</template>
