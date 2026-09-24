<script setup lang="ts">
import { computed, onMounted, ref } from 'vue';
import { t } from '../i18n';
import { addPage, automaticIcon, liveOf, state, topbarMax } from '../store';
import { emptyPage, instanceId } from '../model/pages';
import { tilePalette } from '../model/tile-palette';
import { glyph } from '../model/topbar';
import type { HeaderItem } from '../types';
import HelpTip from './HelpTip.vue';

const emit = defineEmits<{ close: [] }>();
const dialog = ref<HTMLDialogElement>();
const title = ref(''), home = ref(true), clock = ref(true), query = ref('');
const chosen = ref<string[]>([]);
const count = computed(() => chosen.value.length + Number(clock.value));
const matches = computed(() => {
  const search = query.value.trim().toLocaleLowerCase();
  return state.inventory.entities.filter(entity => `${entity.name} ${entity.id} ${entity.area || ''}`.toLocaleLowerCase().includes(search));
});
const invalidTitle = computed(() => new TextEncoder().encode(title.value.trim()).length > 96);
function create() {
  if (invalidTitle.value || count.value > topbarMax()) return;
  const page = emptyPage();
  page.topbar.title = title.value.trim() ? { source: 'text', text: title.value.trim() } : { source: 'screen' };
  if (!home.value) page.topbar.leading = [];
  page.topbar.trailing = [
    ...(clock.value ? [{ id: instanceId(), type: 'clock' } as HeaderItem & { id: string }] : []),
    ...chosen.value.map(entity => ({ id: instanceId(), type: 'entity', entity, content: 'state', icon: 'auto', show: 'always' } as HeaderItem & { id: string })),
  ];
  if (addPage(page.topbar)) emit('close');
}
onMounted(() => dialog.value?.showModal());
</script>

<template>
  <dialog ref="dialog" class="page-wizard" aria-labelledby="page-wizard-title" @cancel.prevent="emit('close')">
    <form @submit.prevent="create">
      <header><h2 id="page-wizard-title">{{ t('editor.layout.add_page') }}</h2>
        <button class="icon-btn" type="button" :aria-label="t('editor.common.close')" @click="emit('close')">✕</button>
      </header>
      <div class="f">
        <div class="f-label"><label for="new-page-title">{{ t('editor.pages.title') }}</label><HelpTip :text="t('editor.pages.title_hint')" /></div>
        <input id="new-page-title" v-model="title" :placeholder="state.document?.title" :aria-invalid="invalidTitle" autofocus />
        <small v-if="invalidTitle" class="warn">{{ t('addon.errors.layout.page_title') }}</small>
      </div>
      <div class="bar-choices">
        <label><input v-model="home" type="checkbox" /> <span class="mdi">{{ glyph('F02DC') }}</span>{{ t('editor.pages.home_control') }}</label>
        <label><input v-model="clock" type="checkbox" :disabled="!clock && count >= topbarMax()" /> <span class="mdi">{{ glyph('F0150') }}</span>{{ t('editor.pages.clock_control') }}</label>
      </div>
      <div class="f">
        <label class="f-label" for="new-page-entity">{{ t('editor.topbar.add.entity') }} <small>{{ count }}/{{ topbarMax() }}</small></label>
        <div v-if="chosen.length" class="chosen">
          <button v-for="id in chosen" :key="id" :aria-label="`${t('editor.common.remove')}: ${state.inventory.entities.find(entity => entity.id === id)?.name || id}`" class="btn mini" type="button" @click="chosen = chosen.filter(item => item !== id)">
            {{ state.inventory.entities.find(entity => entity.id === id)?.name || id }} <span aria-hidden="true">×</span>

          </button>
        </div>
        <input id="new-page-entity" v-model="query" type="search" :placeholder="t('editor.topbar.add.search')" />
        <div class="entity-options">
          <label v-for="entity in matches.slice(0, 40)" :key="entity.id">
            <input v-model="chosen" type="checkbox" :value="entity.id" :disabled="!chosen.includes(entity.id) && count >= topbarMax()" />
            <span class="mdi entity-icon" :style="{ color: tilePalette(entity.id, liveOf(entity.id)).icon, background: tilePalette(entity.id, liveOf(entity.id)).circle }">{{ glyph(automaticIcon(entity.id)) }}</span>
            <span><b>{{ entity.name }}</b><small>{{ [entity.area, entity.id].filter(Boolean).join(' · ') }}</small></span>
          </label>
          <small v-if="!matches.length">{{ t('editor.topbar.add.none_found') }}</small>
          <small v-else-if="matches.length > 40">{{ t('editor.common.results', matches.length) }}</small>
        </div>
      </div>
      <footer><button type="button" class="btn" @click="emit('close')">{{ t('editor.common.cancel') }}</button>
        <button type="submit" class="btn primary" :disabled="invalidTitle || count > topbarMax()">{{ t('editor.layout.add_page') }}</button></footer>
    </form>
  </dialog>
</template>

<style scoped>
.page-wizard { width: 480px; max-width: calc(100vw - 24px); max-height: calc(100dvh - 24px); overflow: auto; padding: 24px; border: 1px solid var(--line); border-radius: 16px; background: var(--surface); color: var(--ink); }
.page-wizard::backdrop { background: rgb(0 0 0 / .5); }
header, footer, .bar-choices, .chosen { display: flex; align-items: center; gap: 10px; }
header { justify-content: space-between; margin-bottom: 20px; } h2 { margin: 0; font-size: 20px; }
footer { justify-content: flex-end; margin-top: 20px; }
.bar-choices, .chosen { flex-wrap: wrap; margin-bottom: 16px; }
.bar-choices label, .entity-options label { display: flex; align-items: center; gap: 8px; }
input[type=checkbox] { width: auto; flex: none; }
.entity-options { max-height: 220px; overflow-y: auto; margin-top: 10px; }
.entity-options label { padding: 10px 2px; }
.entity-options label > span:last-child { min-width: 0; }
.entity-options small { display: block; color: var(--muted); overflow-wrap: anywhere; }
.entity-icon { display: grid; place-items: center; width: 32px; height: 32px; border-radius: 50%; flex: none; }
.mdi { color: var(--accent); font-size: 20px; }
.f-label { display: flex; align-items: center; gap: 8px; }
</style>
