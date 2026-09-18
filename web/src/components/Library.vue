<script setup lang="ts">
// The entities a tile can show, with a search and one filter per domain. A click adds the entity to the
// marked empty cell or the first free one; a drag puts it exactly where it lands.
import { computed } from "vue";
import { vDrag } from "../drag";
import { domainInfo } from "../model/layout";
import { glyph } from "../model/topbar";
import { addTile, automaticIcon, isGuition, state, tileLimit } from "../store";

const FILTERS: [string, string][] = [
  ["", "All"], ["light", "Lights"], ["climate", "Climate"], ["switch", "Switches"], ["binary_sensor", "Status"], ["button", "Actions"],
  ["script", "Scripts"], ["fan", "Fans"], ["cover", "Covers"], ["scene", "Scenes"], ["vacuum", "Vacuum"], ["sensor", "Sensors"],
  ["media_player", "Media"], ["weather", "Weather"], ["number", "Values"], ["select", "Selects"], ["person", "People"], ["timer", "Timers"], ["screen", "Clock"],
];
const ALIAS: Record<string, string> = { switch: "input_boolean", number: "input_number", select: "input_select", weather: "sun", button: "input_button" };
const chosen = computed(() => new Set(state.layout?.tiles.map((t) => t.entity) || []));
const matches = computed(() => {
  const query = state.search.toLocaleLowerCase(), filter = state.filter;
  // The picker offers what a tile can show; camera images need a Guition (app 0.2.66).
  return [...(state.inventory.builtin || []), ...state.inventory.entities].filter((e) =>
    e.tile !== false &&
    (isGuition.value || !["camera", "image"].includes(e.id.split(".")[0])) &&
    (!filter || e.id.startsWith(filter + ".") || ALIAS[filter] === e.id.split(".")[0]) &&
    `${e.name} ${e.id} ${e.device || ""} ${e.area || ""}`.toLocaleLowerCase().includes(query));
});
const full = computed(() => (state.layout?.tiles.length || 0) >= tileLimit.value);
const count = computed(() => state.inventory.entities.length);
</script>

<template>
  <aside class="library" id="library">
    <div class="lib-head">
      <div class="lib-title">Library <small>{{ count }} entities</small></div>
      <input id="search" v-model="state.search" type="search" placeholder="Search by name, device, or room…" autocomplete="off" aria-label="Add an entity" />
      <div class="filters" id="filters">
        <button v-for="[value, label] in FILTERS" :key="value" type="button" :aria-pressed="state.filter === value ? 'true' : 'false'" @click="state.filter = value">
          <span v-if="value" class="domain-icon" :style="{ color: domainInfo(value + '.')[2], background: domainInfo(value + '.')[3] }" aria-hidden="true">{{ domainInfo(value + ".")[1] }}</span>{{ label }}
        </button>
      </div>
    </div>
    <div class="lib-list" id="results" aria-live="polite">
      <button v-for="entity in matches.slice(0, 80)" :key="entity.id" type="button" class="ent" :title="entity.id"
        :disabled="chosen.has(entity.id) || full" v-drag="{ kind: 'entity', id: entity.id }" @click="addTile(entity.id)">
        <span class="av mdi" :style="{ color: domainInfo(entity.id)[2], background: domainInfo(entity.id)[3] }">{{ state.inventory.icons ? glyph(automaticIcon(entity.id)) : domainInfo(entity.id)[1] }}</span>
        <span class="tx">
          <b>{{ entity.name }}</b>
          <small>{{ [domainInfo(entity.id)[0], entity.area, entity.device].filter(Boolean).join(" · ") }}</small>
        </span>
        <span class="add" :class="{ done: chosen.has(entity.id) }">{{ chosen.has(entity.id) ? "✓" : "+" }}</span>
      </button>
      <p v-if="!matches.length" class="hint">No entities found. Try a different name.</p>
      <p v-else-if="matches.length > 80" class="hint">{{ matches.length }} results. Keep typing to narrow it down.</p>
    </div>
    <div class="lib-foot">{{ full ? `This screen holds ${tileLimit} tiles${tileLimit === 10 ? "; update its firmware for 20" : ""}.` : "Drag onto a page, or tap + to add to the first free slot." }}</div>
  </aside>
</template>
