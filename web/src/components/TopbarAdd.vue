<script setup lang="ts">
// Adding to the top bar: the screen's own items, Home Assistant's suggestions, or any entity.
import { computed, ref } from "vue";
import { BUILTIN_ICONS, clockText, dateText, glyph, itemKey } from "../model/topbar";
import { addTopbarItem, automaticIcon, closeInspector, iconNamed, settingValues, state, topbarItems, topbarMax } from "../store";
import type { HeaderItem } from "../types";

const query = ref("");
const taken = computed(() => new Set(topbarItems().map(itemKey)));
const clock24 = computed(() => settingValues().clock_24h !== false);
const samples = computed(() => ({ clock: clockText(clock24.value, new Date(state.now)), analog: "Small dial with the time", date: dateText(new Date(state.now)) } as Record<string, string>));
const suggested = computed(() => state.inventory.header?.suggestions?.[state.selected || ""] || []);
const matches = computed(() => {
  const q = query.value.trim().toLocaleLowerCase();
  return state.inventory.entities.filter((e) => `${e.name} ${e.id} ${e.area || ""} ${e.device || ""}`.toLocaleLowerCase().includes(q));
});
const entityItem = (id: string): HeaderItem => ({ type: "entity", entity: id, content: "state", icon: "auto", show: "always" });
</script>

<template>
  <div class="dr-head">
    <span class="av" style="background: var(--seg); color: var(--ink-2); font-size: 18px">＋</span>
    <span class="tx"><b>Add to the top bar</b><small>{{ topbarItems().length }} of {{ topbarMax() }} slots used</small></span>
    <button type="button" class="icon-btn" aria-label="Close" @click="closeInspector">✕</button>
  </div>
  <div class="dr-body">
    <div class="f">
      <span class="f-label">From the screen itself</span>
      <div class="options">
        <button v-for="b in state.inventory.header?.builtin || []" :key="b.type" type="button" class="option" :disabled="taken.has(itemKey({ type: b.type }))" @click="addTopbarItem({ type: b.type })">
          <span class="mdi">{{ glyph(iconNamed(BUILTIN_ICONS[b.type])?.cp || "F0150") }}</span>
          <span class="tx"><strong>{{ b.label }}</strong><small>{{ taken.has(itemKey({ type: b.type })) ? "Already added" : samples[b.type] }}</small></span>
        </button>
      </div>
    </div>
    <div v-if="suggested.length" class="f">
      <span class="f-label">Suggestions from Home Assistant</span>
      <div class="options">
        <button v-for="s in suggested" :key="itemKey(s.item)" type="button" class="option" :disabled="taken.has(itemKey(s.item))" @click="addTopbarItem(s.item)">
          <span class="mdi">{{ glyph(s.icon || automaticIcon(s.item.entity!)) }}</span>
          <span class="tx"><strong>{{ s.label }}</strong><small>{{ taken.has(itemKey(s.item)) ? "Already added" : [s.name, s.area].filter(Boolean).join(" · ") }}</small></span>
        </button>
      </div>
    </div>
    <div class="f">
      <label class="f-label" for="topbar-search">An entity</label>
      <input id="topbar-search" v-model="query" type="search" placeholder="Search by name, room, or entity, e.g. temperature or door" aria-label="Search for an entity for the top bar" />
      <div class="options">
        <button v-for="e in matches.slice(0, 40)" :key="e.id" type="button" class="option" :disabled="taken.has(itemKey(entityItem(e.id)))" @click="addTopbarItem(entityItem(e.id))">
          <span class="mdi">{{ glyph(automaticIcon(e.id)) }}</span>
          <span class="tx"><strong>{{ e.name }}</strong><small>{{ [e.area, e.id].filter(Boolean).join(" · ") }}</small></span>
        </button>
        <p v-if="!matches.length" class="hint">No entities found.</p>
        <p v-else-if="matches.length > 40" class="hint">{{ matches.length }} results. Keep typing to narrow it down.</p>
      </div>
    </div>
  </div>
  <div class="dr-foot">
    <span class="spacer"></span>
    <button type="button" class="btn quiet" @click="closeInspector">Cancel</button>
  </div>
</template>
