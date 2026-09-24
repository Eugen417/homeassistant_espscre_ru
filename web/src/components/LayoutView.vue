<script setup lang="ts">
// The pages side by side, like swiping on the screen, and the library on the right.
import { computed, onBeforeUnmount, onMounted, ref } from "vue";
import { t } from "../i18n";
import { entriesOf, grid, hasGaps, pageCount } from "../model/layout";
import { addPage, closeInspector, currentScreen, deviceStyle, gridChanged, isCompact, pageReachWarning, pagesShown, redo, reviewScreenGrid, setEditorMode, startUpdate, state, supports, tileLimit, undo } from "../store";
import DevicePage from "./DevicePage.vue";
import Library from "./Library.vue";
import PageMap from "./PageMap.vue";
import NavigationPreview from './NavigationPreview.vue';
import GridReview from './GridReview.vue';
import { resolveLayoutConflict } from '../store';
import { titleOf } from '../model/pages';
const preview = ref(false);
const narrow = ref(window.innerWidth <= 700);
const resize = () => { narrow.value = window.innerWidth <= 700; };
onMounted(() => window.addEventListener('resize', resize));
onBeforeUnmount(() => window.removeEventListener('resize', resize));
const simplePages = computed(() => narrow.value
  ? [Math.max(0, state.document?.pages.findIndex((page) => page.id === state.selectedPageId) ?? 0) + 1]
  : Array.from({ length: shown.value }, (_, index) => index + 1));

const layout = computed(() => state.layout!);
const entries = computed(() => state.drag.preview || entriesOf(layout.value));
const pages = computed(() => pageCount(entries.value, layout.value.pages));
const shown = computed(() => pagesShown());
const canAdd = computed(() => pages.value < grid.pages);
const focused = computed(() => state.document?.pages.findIndex((page) => page.id === state.focusedPageId) ?? -1);
const positionsHint = computed(() => hasGaps(layout.value.tiles) && !supports(0, 2, 26)
  ? t("editor.layout.positions_hint", { firmware: currentScreen.value?.firmware || t("editor.common.unknown") })
  : "");
// Page buttons and swiping off: a page no Go to page tile reaches, or one without a way back.
const reachHint = computed(() => pageReachWarning());
function onCanvasClick(e: MouseEvent) {
  // A click beside the pages closes the drawer; the cards and the bar handle their own clicks.
  if ((e.target as HTMLElement).closest(".device, .page-label, .canvas-head, .map-node, .map-links, button, select, input")) return;
  if (state.inspector) closeInspector();
}
</script>

<template>
  <div class="canvas" id="canvas" @click="onCanvasClick">
    <div v-if="!state.layout" class="page-notice" role="status">
      {{ currentScreen?.page_document?.format === 'legacy-v1' ? currentScreen.page_document.migrationError : t('editor.pages.wait_grid') }}
    </div>
    <template v-else>
    <div class="editor-toolbar">
      <div class="seg" role="group" :aria-label="t('editor.pages.mode')">
        <button type="button" :aria-pressed="state.editorMode === 'simple'" @click="setEditorMode('simple')">{{ t('editor.pages.simple') }}</button>
        <button type="button" :aria-pressed="state.editorMode === 'advanced'" @click="setEditorMode('advanced')">{{ t('editor.pages.advanced') }}</button>
      </div>
      <span class="spacer"></span>
      <button type="button" class="btn mini" @click="preview = true">{{ t('editor.pages.try_navigation') }}</button>
      <button type="button" class="btn mini" :disabled="!state.undoCount" @click="undo">{{ t('editor.common.undo') }}</button>
      <button type="button" class="btn mini" :disabled="!state.redoCount" @click="redo">{{ t('editor.pages.redo') }}</button>
      <button v-if="state.editorMode === 'advanced'" type="button" class="btn mini" :disabled="!canAdd" @click="addPage">{{ t('editor.layout.add_page') }}</button>
    </div>
    <div v-if="currentScreen?.page_capability === 'offline'" class="page-notice" role="status">{{ t('editor.pages.offline_notice') }}</div>
    <div v-if="currentScreen?.page_capability === 'update_screen'" class="page-notice" role="status">
      <span>{{ t('editor.pages.update_notice') }}</span>
      <button v-if="currentScreen?.online && currentScreen.update?.profile" type="button" class="btn mini" @click="startUpdate(currentScreen)">{{ t('editor.screen_view.menu.update') }}</button>
    </div>
    <div v-if="state.conflict" class="page-notice conflict" role="alert">
      <span>{{ t('editor.pages.conflict') }}</span>
      <button type="button" class="btn mini" :disabled="state.busy" @click="resolveLayoutConflict('reload')">{{ t('editor.pages.reload_saved') }}</button>
      <button type="button" class="btn mini" :disabled="state.busy" @click="resolveLayoutConflict('keep')">{{ t('editor.pages.keep_mine') }}</button>
    </div>
    <div v-if="gridChanged" class="page-notice" role="status"><span>{{ t('editor.pages.grid_changed') }}</span><button class="btn mini" @click="reviewScreenGrid">{{ t('editor.pages.grid_review') }}</button></div>
    <div class="canvas-head">
      <b id="count">{{ t("editor.layout.count", { tiles: layout.tiles.length, limit: tileLimit }, pages) }}</b>
      <span v-if="!layout.tiles.length" id="no-tiles">{{ t("editor.layout.no_tiles") }}</span>
      <span v-else>{{ t("editor.layout.how_to") }}</span>
      <span v-if="pages > 1" id="how-to-pages">{{ t("editor.layout.how_to_pages") }}</span>
      <span v-if="positionsHint" id="positions-hint" class="warn">{{ positionsHint }}</span>
      <span v-if="reachHint" id="page-reach-hint" class="warn">{{ reachHint }}</span>
    </div>
    <template v-if="state.editorMode === 'advanced' && focused >= 0">
      <button type="button" class="btn" @click="state.focusedPageId = null">{{ t('editor.pages.back_map') }}</button>
      <div class="pages focused-page"><DevicePage :page="focused" :entries="entries" :pages="pages" :moving="state.drag.moving" map /></div>
    </template>
    <PageMap v-else-if="state.editorMode === 'advanced'" :key="String(narrow)" :compact="narrow" />
    <div v-else class="pages" id="layout-preview" :aria-label="t('editor.layout.aria')">
      <label v-if="narrow" class="mobile-page-picker">{{ t('editor.pages.choose_page') }}<select v-model="state.selectedPageId">
        <option v-for="(page, index) in state.document!.pages" :key="page.id" :value="page.id">{{ index + 1 }} · {{ titleOf(state.document!, page) }}</option>
      </select></label>
      <DevicePage v-for="page in simplePages" :key="state.document?.pages[page - 1]?.id || page" :page="page - 1" :entries="entries" :pages="pages" :moving="state.drag.moving" />
      <button v-if="narrow" class="btn" :disabled="!canAdd" @click="addPage">{{ t('editor.layout.add_page') }}</button>
      <div v-else class="page ghost" :style="deviceStyle" :class="{ disabled: !canAdd }">
        <div class="page-label"><span>{{ t("editor.page.label", { page: shown + 1 }) }}</span></div>
        <div class="device" :class="{ compact: isCompact }" :style="deviceStyle" id="add-page" role="button" :tabindex="canAdd ? 0 : -1" @click="canAdd && addPage()" @keydown.enter.prevent="canAdd && addPage()">
          {{ canAdd ? t("editor.layout.add_page") : t("editor.layout.max_pages", grid.pages) }}
        </div>
      </div>
    </div>
    </template>
  </div>
  <Library v-if="state.layout" />
  <NavigationPreview v-if="preview && state.document" :key="state.selected || ''" @close="preview = false" />
  <GridReview v-if="state.gridReview" />
</template>

<style scoped>
.editor-toolbar { display: flex; align-items: center; gap: 8px; margin-bottom: 18px; }
.page-notice { display: flex; align-items: center; justify-content: space-between; gap: 16px; margin-bottom: 16px; padding: 12px 14px; border: 1px solid var(--line); border-radius: 10px; background: var(--surface); color: var(--muted); }
.conflict { border-color: var(--warn); color: var(--warn); }
.focused-page { padding: 24px 0; justify-content: center; }
.mobile-page-picker { width: 100%; display: grid; gap: 8px; color: var(--muted); }
@media (max-width: 700px) { .editor-toolbar { flex-wrap: wrap; } #layout-preview { flex-direction: column; align-items: center; overflow: visible; } }
</style>
