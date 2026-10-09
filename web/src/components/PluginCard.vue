<script setup lang="ts">
// One plugin as a card: its icon, its maker, one line of what it does, its label and one line of state. The Plugins
// page and a screen's Plugins tab draw the same card; only the state they hand it differs.
import { t } from "../i18n";
import { text, type Plugin } from "../model/plugins";
import { glyph } from "../model/topbar";
import { labelOf, stageOf, type Status } from "../plugin-state";
import Icon from "./ui/Icon.vue";

defineProps<{ plugin: Plugin; status: Status; chosen: boolean }>();
defineEmits<{ open: [] }>();
</script>

<template>
  <button type="button" role="listitem" class="plugin-card" :class="[status.kind, { chosen }]" :data-plugin="plugin.id" @click="$emit('open')">
    <span class="plugin-icon" :class="{ tessera: plugin.tessera }" aria-hidden="true"><span class="mdi">{{ glyph(plugin.icon) }}</span></span>
    <span class="plugin-words">
      <b>{{ text(plugin.name) }}</b>
      <small>{{ plugin.tessera ? t("editor.plugins.from_tessera") : t("editor.plugins.by", { maker: plugin.maintainer }) }}</small>
    </span>
    <span v-if="text(plugin.summary)" class="plugin-summary">{{ text(plugin.summary) }}</span>
    <span class="plugin-foot">
      <em class="plugin-chip" :class="labelOf(plugin)">{{ t(`editor.plugins.label.${labelOf(plugin)}`) }}</em>
      <em v-if="stageOf(plugin)" class="plugin-chip" :class="stageOf(plugin)" :title="t(`editor.plugins.stage_hint.${stageOf(plugin)}`)">{{ t(`editor.plugins.stage.${stageOf(plugin)}`) }}</em>
      <span v-if="status.label" class="plugin-state" :class="status.kind">
        <Icon v-if="status.kind === 'installed'" name="check" />
        <Icon v-else-if="status.kind === 'update'" name="update" />
        <span v-else-if="status.kind === 'building'" class="spin" aria-hidden="true"></span>
        {{ status.label }}
      </span>
    </span>
  </button>
</template>
