<script setup lang="ts">
// One page of the screen as the mockup draws it: the top bar and a 2 × 3 grid of cells.
import { computed } from "vue";
import { t } from "../i18n";
import { cellsOf, pageOf, sizeOf, SLOTS_PER_PAGE, spanOf } from "../model/layout";
import { deviceStyle, isCompact, openBar, removePage, state } from "../store";
import type { Tile } from "../types";
import TileCard from "./TileCard.vue";
import TopbarSvg from "./TopbarSvg.vue";

const props = defineProps<{ page: number; entries: { tile: Tile; slot: number }[]; pages: number; moving: Tile | null }>();
const bySlot = computed(() => new Map(props.entries.map((e) => [e.slot, e])));
const covered = computed(() => new Set(props.entries.flatMap((e) => cellsOf(e.slot, sizeOf(e.tile)).slice(1))));
const cells = computed(() => Array.from({ length: SLOTS_PER_PAGE }, (_, cell) => props.page * SLOTS_PER_PAGE + cell).filter((slot) => !covered.value.has(slot)));
const empty = computed(() => props.pages > 1 && !props.entries.some((e) => pageOf(e.slot) === props.page));
const barSelected = computed(() => state.inspector?.kind === "bar" || state.inspector?.kind === "bar-add");
const filled = computed(() => props.entries.filter((e) => pageOf(e.slot) === props.page).reduce((n, e) => n + spanOf(sizeOf(e.tile)), 0));
function pickCell(slot: number) {
  const marked = state.insertAt === slot;
  state.insertAt = marked ? -1 : slot;
  if (state.insertAt >= 0) document.querySelector<HTMLInputElement>("#search")?.focus();
}
</script>

<template>
  <div class="page">
    <div class="page-label">
      <span>{{ t("editor.page.label", { page: page + 1 }) }}</span>
      <button v-if="empty" type="button" class="btn mini" :title="t('editor.page.remove_title')" @click="removePage(page)">{{ t("editor.page.remove") }}</button>
      <span v-else>{{ filled }} / {{ SLOTS_PER_PAGE }}</span>
    </div>
    <div class="device" :class="{ cyd: isCompact }" :style="deviceStyle">
      <div class="bar-wrap" :class="{ selected: barSelected }" :title="t('editor.page.edit_bar')" role="button" tabindex="0"
        @click="openBar(0)" @keydown.enter.prevent="openBar(0)">
        <TopbarSvg />
      </div>
      <div class="tiles">
        <template v-for="slot in cells" :key="slot">
          <TileCard v-if="bySlot.get(slot)" :tile="bySlot.get(slot)!.tile" :slot="slot" :placeholder="bySlot.get(slot)!.tile === moving" />
          <button v-else type="button" class="cell" :class="{ 'insert-here': state.insertAt === slot }" :data-slot="slot"
            :title="t('editor.page.cell.title')"
            :aria-label="t('editor.page.cell.aria', { slot: (slot % SLOTS_PER_PAGE) + 1, page: page + 1 })" @click="pickCell(slot)">
            <span>+</span><small>{{ state.insertAt === slot ? t("editor.page.cell.next") : t("editor.page.cell.empty") }}</small>
          </button>
        </template>
      </div>
    </div>
  </div>
</template>
