<script setup lang="ts">
import { computed } from "vue";
import { t } from "../i18n";
import { connections, titleOf } from "../model/pages";
import { glyph } from "../model/topbar";
import { closeInspector, duplicateEditorPage, movePage, moveWorkspacePage, openBar, openTile, pageReady, removePage,
  setHomePage, setPageExcluded, setPageHomeControl, setPageTitle, state, workspacePositions } from "../store";

const props = defineProps<{ id: string }>();
const page = computed(() => state.document?.pages.find((item) => item.id === props.id));
const index = computed(() => state.document?.pages.findIndex((item) => item.id === props.id) ?? -1);
const home = computed(() => state.document?.homePageId === props.id);
const point = computed(() => workspacePositions()[props.id] || { x: 0, y: 0 });
const routes = computed(() => state.document ? connections(state.document).filter((route) => route.from === props.id || route.to === props.id) : []);
const name = (id: string) => { const page = state.document?.pages.find((item) => item.id === id); return page ? titleOf(state.document!, page) : ""; };
const canCopy = computed(() => page.value?.tiles.every((tile) => tile.content.kind === "navigation"));
function editRoute(tileId: string) { const tile = state.layout?.tiles.find((item) => item.id === tileId); if (tile) openTile(tile); }
</script>

<template>
  <template v-if="page">
    <div class="dr-head">
      <span class="av mdi">{{ glyph(home ? 'F02DC' : 'F0379') }}</span>
      <span class="tx"><b>{{ name(id) }}</b><small>{{ t('editor.page.label', { page: index + 1 }) }}</small></span>
      <button class="icon-btn" type="button" :aria-label="t('editor.common.close')" @click="closeInspector">✕</button>
    </div>
    <div class="dr-body">
      <div class="f">
        <label class="f-label" for="owned-page-title">{{ t('editor.pages.title') }}</label>
        <input id="owned-page-title" :value="page.topbar.title.source === 'text' ? page.topbar.title.text : ''" :placeholder="state.document?.title"
          @input="setPageTitle(index, ($event.target as HTMLInputElement).value)" />
        <small>{{ t('editor.pages.title_hint') }}</small>
      </div>
      <button type="button" class="btn" :disabled="home || !pageReady" @click="setHomePage(id)">
        <span class="mdi">{{ glyph('F02DC') }}</span> {{ t(home ? 'editor.pages.is_home' : 'editor.pages.set_home') }}
      </button>
      <label class="page-check"><input type="checkbox" :checked="page.navigation.excludeFromPagination" :disabled="!pageReady"
        @change="setPageExcluded(id, ($event.target as HTMLInputElement).checked)" />{{ t('editor.pages.exclude') }}</label>
      <small>{{ t('editor.pages.exclude_hint') }}</small>
      <label class="page-check"><input type="checkbox" :checked="!!page.topbar.leading.length" :disabled="!pageReady"
        @change="setPageHomeControl(id, ($event.target as HTMLInputElement).checked)" />{{ t('editor.pages.home_control') }}</label>
      <button type="button" class="btn" @click="openBar(0, index)">{{ t('editor.pages.edit_topbar') }}</button>
      <div class="f">
        <label class="f-label" for="page-order">{{ t('editor.pages.order') }}</label>
        <select id="page-order" :value="index" @change="movePage(index, Number(($event.target as HTMLSelectElement).value))">
          <option v-for="(_, at) in state.document?.pages" :key="at" :value="at">{{ t('editor.page.label', { page: at + 1 }) }}</option>
        </select>
        <small>{{ t('editor.pages.order_hint') }}</small>
      </div>
      <template v-if="state.editorMode === 'advanced'">
        <div class="f">
          <span class="f-label">{{ t('editor.pages.map_position') }}</span>
          <div class="map-move" role="group" :aria-label="t('editor.pages.map_position')">
            <button v-for="[dx, dy, key, icon] in [[-1, 0, 'left', 'F004D'], [0, -1, 'up', 'F005D'], [0, 1, 'down', 'F0045'], [1, 0, 'right', 'F0054']]"
              :key="String(key)" class="btn mdi" type="button" :aria-label="t(`editor.pages.${key}`)"
              @click="moveWorkspacePage(id, point.x + Number(dx), point.y + Number(dy))">{{ glyph(String(icon)) }}</button>
          </div>
        </div>
        <div class="f"><span class="f-label">{{ t('editor.pages.routes') }}</span>
          <button v-for="route in routes" :key="route.tileId" class="route-row" type="button" @click="editRoute(route.tileId)">
            {{ name(route.from) }} → {{ name(route.to) }}
          </button>
          <small v-if="!routes.length">{{ t('editor.pages.no_routes') }}</small>
        </div>
        <button type="button" class="btn primary" @click="state.focusedPageId = id; closeInspector()">{{ t('editor.pages.edit_page') }}</button>
      </template>
      <div class="f">
        <button class="btn" type="button" :disabled="!canCopy" :title="canCopy ? '' : t('editor.pages.copy_conflict')" @click="duplicateEditorPage(id, false)">{{ t('editor.pages.duplicate') }}</button>
        <small v-if="!canCopy">{{ t('editor.pages.copy_conflict') }}</small>
        <button class="btn" type="button" @click="duplicateEditorPage(id, true)">{{ t('editor.pages.empty_copy') }}</button>
      </div>
    </div>
    <div class="dr-foot"><button class="btn danger" type="button" :disabled="state.document!.pages.length < 2" @click="removePage(index)">{{ t('editor.page.remove') }}</button></div>
  </template>
</template>

<style scoped>
.page-check { display: flex; align-items: start; gap: 10px; margin-top: 18px; }
.page-check input { width: auto; margin-top: 3px; }
.map-move { display: flex; gap: 8px; }
.route-row { text-align: left; padding: 10px 0; color: var(--accent); border: 0; background: none; }
</style>
