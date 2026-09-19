<script setup lang="ts">
// The entities a tile can show, with a search, a filter per domain and per room, and a switch that hides what is
// already on the screen. A click adds the entity to the marked empty cell or the first free one; a drag puts it
// exactly where it lands.
import { computed } from "vue";
import { vDrag } from "../drag";
import { t } from "../i18n";
import { domainInfo } from "../model/layout";
import { glyph } from "../model/topbar";
import { addTile, automaticIcon, isGuition, repeatable, state, tileLimit } from "../store";

// The domains to filter on; the label of each is editor.library.filters.<domain>, "all" for no filter.
const FILTERS = [
  "", "light", "climate", "switch", "binary_sensor", "button", "script", "fan", "cover", "scene", "vacuum", "sensor",
  "media_player", "weather", "number", "select", "person", "timer", "screen",
];
const ALIAS: Record<string, string> = { switch: "input_boolean", number: "input_number", select: "input_select", weather: "sun", button: "input_button" };
const chosen = computed(() => new Set(state.layout?.tiles.map((t) => t.entity) || []));
// On the screen and not to be added again; a page tile can be, when the firmware takes several (0.2.65).
const placed = (id: string) => chosen.value.has(id) && !repeatable(id);
const rooms = computed(() => [...new Set(state.inventory.entities.map((e) => e.area).filter((a): a is string => Boolean(a)))].sort((a, b) => a.localeCompare(b)));
const matches = computed(() => {
  const query = state.search.toLocaleLowerCase(), filter = state.filter, room = state.room;
  // The picker offers what a tile can show; camera images need a Guition (app 0.2.66).
  return [...(state.inventory.builtin || []), ...state.inventory.entities].filter((e) =>
    e.tile !== false &&
    (isGuition.value || !["camera", "image"].includes(e.id.split(".")[0])) &&
    (!filter || e.id.startsWith(filter + ".") || ALIAS[filter] === e.id.split(".")[0]) &&
    (!room || e.area === room) &&
    (!state.hidePlaced || !placed(e.id)) &&
    `${e.name} ${e.id} ${e.device || ""} ${e.area || ""}`.toLocaleLowerCase().includes(query));
});
const full = computed(() => (state.layout?.tiles.length || 0) >= tileLimit.value);
const count = computed(() => state.inventory.entities.length);
// The avatar shows the state at a glance: lit for on, grey for an entity Home Assistant can't reach.
const tone = (e: { id: string; state?: string }) => {
  const domain = e.id.split(".")[0];
  if (e.state === "unavailable" || e.state === "unknown") return "gone";
  if (["light", "switch", "input_boolean", "fan"].includes(domain) && e.state === "on") return "on";
  return "";
};
</script>

<template>
  <aside class="library" id="library">
    <div class="lib-head">
      <div class="lib-title">{{ t("editor.library.title") }} <small>{{ t("editor.library.entities", count) }}</small></div>
      <input id="search" v-model="state.search" type="search" :placeholder="t('editor.library.search')" autocomplete="off" :aria-label="t('editor.library.search_label')" />
      <div class="filters" id="filters">
        <button v-for="value in FILTERS" :key="value" type="button" :aria-pressed="state.filter === value ? 'true' : 'false'" @click="state.filter = value">
          <span v-if="value" class="domain-icon" :style="{ color: domainInfo(value + '.')[2], background: domainInfo(value + '.')[3] }" aria-hidden="true">{{ domainInfo(value + ".")[1] }}</span>{{ t(`editor.library.filters.${value || "all"}`) }}
        </button>
      </div>
      <div class="lib-row">
        <select id="room" v-model="state.room" :aria-label="t('editor.library.room')">
          <option value="">{{ t("editor.library.all_rooms") }}</option>
          <option v-for="room in rooms" :key="room" :value="room">{{ room }}</option>
        </select>
        <button type="button" class="chip-toggle" id="hide-placed" :aria-pressed="state.hidePlaced ? 'true' : 'false'" :title="t('editor.library.hide_placed_title')" @click="state.hidePlaced = !state.hidePlaced">{{ t("editor.library.hide_placed") }}</button>
      </div>
    </div>
    <div class="lib-list" id="results" aria-live="polite">
      <button v-for="entity in matches.slice(0, 80)" :key="entity.id" type="button" class="ent" :title="entity.id"
        :disabled="placed(entity.id) || full" v-drag="{ kind: 'entity', id: entity.id }" @click="addTile(entity.id)">
        <span class="av mdi" :class="tone(entity)" :style="{ color: domainInfo(entity.id)[2], background: domainInfo(entity.id)[3] }">{{ state.inventory.icons ? glyph(automaticIcon(entity.id)) : domainInfo(entity.id)[1] }}</span>
        <span class="tx">
          <b>{{ entity.name }}</b>
          <small>{{ [domainInfo(entity.id)[0], entity.area, entity.device].filter(Boolean).join(" · ") }}</small>
        </span>
        <span class="add" :class="{ done: placed(entity.id) }">{{ placed(entity.id) ? "✓" : "+" }}</span>
      </button>
      <p v-if="!matches.length" class="hint">{{ state.hidePlaced && !state.search && !state.filter && !state.room ? t("editor.library.all_placed") : t("editor.library.none_found") }}</p>
      <p v-else-if="matches.length > 80" class="hint">{{ t("editor.common.results", matches.length) }}</p>
    </div>
    <div class="lib-foot">{{ full ? t(tileLimit < 48 ? "editor.library.full_update" : "editor.library.full", tileLimit) : t("editor.library.hint") }}</div>
  </aside>
</template>
