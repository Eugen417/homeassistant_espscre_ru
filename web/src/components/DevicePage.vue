<script setup lang="ts">
// One page of the screen as the mockup draws it: the top bar and the screen's own grid of cells.
import { computed, nextTick } from "vue";
import { vDrag } from "../drag";
import { t } from "../i18n";
import { cellsOf, grid, pageOf, sizeOf, spanOf } from "../model/layout";
import { deviceStyle, isCompact, movePage, openBar, pageTitleShown, removePage, state } from "../store";
import type { Tile } from "../types";
import TileCard from "./TileCard.vue";
import TopbarSvg from "./TopbarSvg.vue";

const props = defineProps<{ page: number; entries: { tile: Tile; slot: number }[]; pages: number; moving: Tile | null }>();
const bySlot = computed(() => new Map(props.entries.map((e) => [e.slot, e])));
const covered = computed(() => new Set(props.entries.flatMap((e) => cellsOf(e.slot, sizeOf(e.tile)).slice(1))));
const cells = computed(() => Array.from({ length: grid.slots }, (_, cell) => props.page * grid.slots + cell).filter((slot) => !covered.value.has(slot)));
const empty = computed(() => props.pages > 1 && !props.entries.some((e) => pageOf(e.slot) === props.page));
const barSelected = computed(() => state.inspector?.kind === "bar" || state.inspector?.kind === "bar-add");
const filled = computed(() => props.entries.filter((e) => pageOf(e.slot) === props.page).reduce((n, e) => n + spanOf(sizeOf(e.tile)), 0));
function pickCell(slot: number) {
  const marked = state.insertAt === slot;
  state.insertAt = marked ? -1 : slot;
  if (state.insertAt >= 0) document.querySelector<HTMLInputElement>("#search")?.focus();
}
// A whole page moves by its label (app 0.2.121). One page has nowhere to go, and the page a tile can start behind
// the last one isn't a page yet.
const movable = computed(() => props.pages > 1 && props.page < props.pages);
// This is the page being carried, drawn in the place it would land.
const carried = computed(() => state.drag.page?.to === props.page);
// Left and right move the page a place along, for a finger on a phone and for anyone who can't drag.
async function onKey(e: KeyboardEvent) {
  const step = ({ ArrowLeft: -1, ArrowRight: 1 } as Record<string, number>)[e.key];
  if (!step) return;
  e.preventDefault();
  const to = props.page + step;
  if (movePage(props.page, to)) {
    await nextTick();
    document.querySelector<HTMLElement>(`.pages [data-page="${to}"]`)?.focus();
  }
}
</script>

<template>
  <div class="page" :class="{ carried }" :style="deviceStyle">
    <div class="page-label">
      <button v-if="movable" type="button" class="grab" :data-page="page" v-drag="{ kind: 'page', page }"
        :title="t('editor.page.move_title')" :aria-label="t('editor.page.move_aria', { page: page + 1 })" @keydown="onKey">
        <span class="grip" aria-hidden="true">⋮⋮</span>{{ t("editor.page.label", { page: page + 1 }) }}
      </button>
      <span v-else>{{ t("editor.page.label", { page: page + 1 }) }}</span>
      <button v-if="empty" type="button" class="btn mini" :title="t('editor.page.remove_title')" @click="removePage(page)">{{ t("editor.page.remove") }}</button>
      <span v-else>{{ filled }} / {{ grid.slots }}</span>
    </div>
    <div class="device" :class="{ cyd: isCompact }">
      <div class="bar-wrap" :class="{ selected: barSelected }" :title="t('editor.page.edit_bar')" role="button" tabindex="0"
        @click="openBar(0, page)" @keydown.enter.prevent="openBar(0, page)">
        <TopbarSvg :name-text="pageTitleShown(page)" />
      </div>
      <div class="tiles">
        <template v-for="slot in cells" :key="slot">
          <TileCard v-if="bySlot.get(slot)" :tile="bySlot.get(slot)!.tile" :slot="slot" :placeholder="bySlot.get(slot)!.tile === moving" />
          <button v-else type="button" class="cell" :class="{ 'insert-here': state.insertAt === slot }" :data-slot="slot"
            :title="t('editor.page.cell.title')"
            :aria-label="t('editor.page.cell.aria', { slot: (slot % grid.slots) + 1, page: page + 1 })" @click="pickCell(slot)">
            <span>+</span><small>{{ state.insertAt === slot ? t("editor.page.cell.next") : t("editor.page.cell.empty") }}</small>
          </button>
        </template>
      </div>
    </div>
  </div>
</template>
