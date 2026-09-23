<script setup lang="ts">
import { ref, watch } from 'vue';
import { t } from '../i18n';
import { titleOf } from '../model/pages';
import { copyPageBars, state } from '../store';
const props = defineProps<{ pageId: string }>();
const source = ref(props.pageId), targets = ref<string[]>([]), whole = ref(false);
watch(() => props.pageId, (id) => { source.value = id; targets.value = []; });
watch(source, (id) => { targets.value = id === props.pageId ? [] : [props.pageId]; });
function copy() { if (copyPageBars(source.value, targets.value, whole.value)) targets.value = []; }
</script>
<template>
  <details class="copy-bar">
    <summary>{{ t('editor.pages.copy_bar') }}</summary>
    <label class="f"><span class="f-label">{{ t('editor.pages.copy_from') }}</span>
      <select v-model="source"><option v-for="(page, index) in state.document!.pages" :key="page.id" :value="page.id">{{ index + 1 }} · {{ titleOf(state.document!, page) }}</option></select>
    </label>
    <label class="f"><span class="f-label">{{ t('editor.pages.copy_content') }}</span>
      <select v-model="whole"><option :value="false">{{ t('editor.pages.copy_items') }}</option><option :value="true">{{ t('editor.pages.copy_whole') }}</option></select>
    </label>
    <fieldset><legend>{{ t('editor.pages.copy_to') }}</legend>
      <label v-for="(page, index) in state.document!.pages" v-show="page.id !== source" :key="page.id" class="copy-target">
        <input v-model="targets" type="checkbox" :value="page.id" />{{ index + 1 }} · {{ titleOf(state.document!, page) }}
      </label>
    </fieldset>
    <button class="btn" type="button" :disabled="!targets.length" @click="copy">{{ t('editor.pages.copy_apply') }}</button>
  </details>
</template>
<style scoped>
.copy-bar { border-block: 1px solid var(--line); padding: 12px 0; margin: 16px 0; }
summary { cursor: pointer; margin-bottom: 12px; }
fieldset { border: 0; padding: 0; margin: 12px 0; }
.copy-target { display: flex; gap: 8px; align-items: center; margin: 8px 0; }
input { width: auto; }
</style>
