<script setup lang="ts">
// One page of the screen as the mockup draws it: the top bar and the screen's own grid of cells.
import { computed, nextTick, onBeforeUnmount } from "vue";
import { vDrag } from "../drag";
import { t } from "../i18n";
import { cellsOf, grid, pageOf, sizeOf, spanOf } from "../model/layout";
import { deviceStyle, homeKeyShown, isCompact, movePage, navigationSettings, openBar, openPage, pageAt, pageReady, pageTitleShown, removePage, screenText, setHomePage, state, topbarItems } from "../store";
import { glyph } from "../model/topbar";
import type { Tile } from "../types";
import TileCard from "./TileCard.vue";
import TopbarSvg from "./TopbarSvg.vue";
import PageNavigation from './PageNavigation.vue';
import type { NavigationIntent } from '../model/pages';

const props = defineProps<{ page: number; entries: { tile: Tile; slot: number }[]; pages: number; moving: Tile | null; map?: boolean; preview?: boolean; canGoBack?: boolean }>();
const emit = defineEmits<{ navigate: [intent: NavigationIntent] }>();
const owned = computed(() => pageAt(props.page));
const isHome = computed(() => owned.value?.id === state.document?.homePageId);
const backInHeader = computed(() => !navigationSettings().pageButtons && Boolean(owned.value?.navigation.excludeFromPagination));
const bySlot = computed(() => new Map(props.entries.map((e) => [e.slot, e])));
const covered = computed(() => new Set(props.entries.flatMap((e) => cellsOf(e.slot, sizeOf(e.tile)).slice(1))));
const cells = computed(() => Array.from({ length: grid.slots }, (_, cell) => props.page * grid.slots + cell).filter((slot) => !covered.value.has(slot)));
const barSelected = computed(() => state.barPage === props.page && (state.inspector?.kind === "bar" || state.inspector?.kind === "bar-add"));
const filled = computed(() => props.entries.filter((e) => pageOf(e.slot) === props.page).reduce((n, e) => n + spanOf(sizeOf(e.tile)), 0));
function pickCell(slot: number) {
  state.selectedPageId = owned.value?.id || state.selectedPageId;
  const marked = state.insertAt === slot;
  state.insertAt = marked ? -1 : slot;
  if (state.insertAt >= 0) document.querySelector<HTMLInputElement>("#search")?.focus();
}
// A whole page moves by its label (app 0.2.121) and leaves by the button beside its cell count (app 0.2.123). One
// page has nowhere to go and cannot leave either, and the page a tile can start behind the last one isn't a page yet.
const movable = computed(() => !props.preview && !props.map && props.pages > 1 && props.page < props.pages);
let cancelHome = () => {};
onBeforeUnmount(() => cancelHome());
function dragHome(event: PointerEvent) {
  if (!isHome.value || !pageReady.value || event.button !== 0) return;
  cancelHome();
  const start = { x: event.clientX, y: event.clientY };
  const finish = (end: PointerEvent) => {
    window.removeEventListener('pointerup', finish); window.removeEventListener('pointercancel', cancel);
    if (Math.hypot(end.clientX - start.x, end.clientY - start.y) < 8) return;
    const id = document.elementFromPoint(end.clientX, end.clientY)?.closest<HTMLElement>('[data-page-id]')?.dataset.pageId;
    if (id) setHomePage(id);
  };
  const cancel = () => { window.removeEventListener('pointerup', finish); window.removeEventListener('pointercancel', cancel); };
  cancelHome = cancel;
  window.addEventListener('pointerup', finish); window.addEventListener('pointercancel', cancel);
}
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
  <div class="page" :class="{ carried }" :style="deviceStyle" :data-page-id="owned?.id">
    <div v-if="!preview" class="page-label">
      <button v-if="movable" type="button" class="grab" :data-page="page" v-drag="{ kind: 'page', page }"
        :title="t('editor.page.move_title')" :aria-label="t('editor.page.move_aria', { page: page + 1 })" @keydown="onKey">
        <span class="grip" aria-hidden="true">⋮⋮</span>{{ t("editor.page.label", { page: page + 1 }) }}
      </button>
      <span v-else>{{ t("editor.page.label", { page: page + 1 }) }}</span>
      <button v-if="owned" type="button" class="home-badge mdi" :class="{ active: isHome }" :disabled="!pageReady && !isHome"
        :aria-label="t(isHome ? 'editor.pages.drag_home' : 'editor.pages.set_home')" :title="t(isHome ? 'editor.pages.drag_home' : 'editor.pages.set_home')"
        @pointerdown.stop="dragHome" @click.stop="!isHome && setHomePage(owned.id)">{{ glyph('F02DC') }}</button>
      <span v-if="owned?.navigation.excludeFromPagination" class="deeplink-badge">{{ t('editor.pages.deeplink') }}</span>
      <span class="page-side">
        <span>{{ filled }} / {{ grid.slots }}</span>
        <button v-if="owned" type="button" class="icon-btn" :aria-label="t('editor.pages.page_settings')" @click="openPage(owned.id)">···</button>
        <button v-if="movable" type="button" class="icon-btn page-remove" :title="t('editor.page.remove_title')"
          :aria-label="t('editor.page.remove_aria', { page: page + 1 })" @click="removePage(page)">✕</button>
      </span>
    </div>
    <div class="device" :class="{ compact: isCompact }">
      <div class="bar-wrap" :class="{ selected: !preview && barSelected }" :title="preview ? undefined : t('editor.page.edit_bar')" :role="preview ? undefined : 'button'" :tabindex="preview ? undefined : 0"
        @click="!preview && openBar(0, page)" @keydown.enter.prevent="!preview && openBar(0, page)">
        <TopbarSvg :items="topbarItems(page)" :name-text="pageTitleShown(page)" :home="homeKeyShown(page)" :back="backInHeader" />
        <button v-if="preview && (backInHeader || homeKeyShown(page))" type="button" class="preview-home" :aria-label="backInHeader ? screenText('screen.navigation.back') : t('editor.pages.go_home')" @click.stop="emit('navigate', { kind: backInHeader ? 'back' : 'home' })"></button>
      </div>
      <div class="tiles">
        <template v-for="slot in cells" :key="slot">
          <TileCard v-if="bySlot.get(slot)" :tile="bySlot.get(slot)!.tile" :slot="slot" :placeholder="bySlot.get(slot)!.tile === moving" :preview="preview" @navigate="emit('navigate', { kind: 'tile', tileId: $event })" />
          <span v-else-if="preview" class="cell preview-empty"></span>
          <button v-else type="button" class="cell" :class="{ 'insert-here': state.insertAt === slot }" :data-slot="slot"
            :title="t('editor.page.cell.title')"
            :aria-label="t('editor.page.cell.aria', { slot: (slot % grid.slots) + 1, page: page + 1 })" @click="pickCell(slot)">
            <span>+</span><small>{{ state.insertAt === slot ? t("editor.page.cell.next") : t("editor.page.cell.empty") }}</small>
          </button>
        </template>
      </div>
      <PageNavigation v-if="owned" :page-id="owned.id" :interactive="preview" :can-go-back="canGoBack" @navigate="emit('navigate', { kind: $event })" />
    </div>
  </div>
</template>

<style scoped>
.page { min-width: 0; }
.page-label { gap: 4px; white-space: nowrap; }
.page-remove:hover { color: var(--danger); background: var(--danger-soft); }
.device { grid-template-rows: auto minmax(0, 1fr); }
.device:has(.page-navigation) { grid-template-rows: auto minmax(0, 1fr) auto; }
.bar-wrap { position: relative; }
.preview-home { position: absolute; inset: 0 auto 0 0; width: 30px; border: 0; background: transparent; }
.preview-empty { visibility: hidden; }
.home-badge { border: 1px solid transparent; background: transparent; color: var(--muted); border-radius: 8px; font-size: 20px; padding: 4px 6px; touch-action: none; }
.home-badge.active { border-color: var(--accent); color: var(--accent); background: var(--surface); cursor: grab; }
.deeplink-badge { font-size: 10px; border: 1px solid var(--line); border-radius: 5px; padding: 3px 5px; }
</style>
