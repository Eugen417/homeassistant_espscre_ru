<script setup lang="ts">
import { ref } from "vue";
import { t } from "../i18n";
import { versionAtLeast } from "../model/layout";
import { glyph } from "../model/topbar";
import {
  copyText, firmwareVersion, go, needsUpdate, newLanguageText, openIntegrations, phaseText, refresh, route, select, startUpdate, state,
  updateProgress, whatsNew,
} from "../store";
import type { Screen } from "../types";

const hostFor = ref<string | null>(null);
const host = ref("");
// A screen that only needs the new language (app 0.2.90) says so instead of naming the version it already has.
const languageOnly = (screen: Screen) => {
  const u = screen.update || {};
  return Boolean(u.language) && (!u.target || versionAtLeast(firmwareVersion(screen), u.target));
};
function updateState(screen: Screen) {
  const u = screen.update || {};
  if (u.state === "running" || state.updating.includes(screen.id)) return { kind: "running", text: phaseText(u.phase) };
  if (u.state === "queued") return { kind: "queued", text: t("editor.sidebar.update.queued") };
  // A screen ESP Screens did not install has no YAML here to build from, so there is nothing to press: say why
  // instead of offering a button that cannot work (the nightly round already passes such a screen by).
  if (needsUpdate(screen) && screen.online && !u.profile)
    return { kind: "blocked", text: t("addon.errors.updates.no_profile") };
  if (needsUpdate(screen) && screen.online)
    return { kind: "available", text: languageOnly(screen) ? newLanguageText() : t("editor.sidebar.update.available", { version: u.target }) };
  if (u.result && Date.now() / 1000 - u.result.time < 86400) return { kind: u.result.state === "failed" ? "failed" : "done", text: u.result.message };
  return null;
}
// What the update brings: the new language first, when the version changes as well, then the firmware's notes.
const notes = (screen: Screen) => [...(screen.update?.language && !languageOnly(screen) ? [newLanguageText()] : []), ...whatsNew(screen)];
function update(screen: Screen) {
  const u = screen.update || {};
  if (u.host && u.profile) startUpdate(screen);
  else if (u.profile) { hostFor.value = screen.id; host.value = ""; }
}
function startWithHost(screen: Screen) {
  const address = host.value.trim();
  if (!address) return;
  hostFor.value = null;
  startUpdate(screen, address);
}
const lastLog = () => {
  const lines = state.firmwareJob?.logs || [];
  return lines.length ? lines[lines.length - 1] : "";
};
const pendingText = (p: { installed?: boolean; downloaded?: boolean; file: string }) => p.installed
  ? t("editor.sidebar.pending.installed")
  : p.downloaded
    ? t("editor.sidebar.pending.downloaded")
    : t("editor.sidebar.pending.not_flashed", { file: p.file });
</script>

<template>
  <aside class="side">
    <div class="brand"><span class="mark">▦</span><span>ESP Screens</span></div>
    <span id="connection" class="conn" :class="{ online: state.connected }" role="status">
      {{ !state.reachable ? t("editor.sidebar.connection.unreachable") : state.connected ? t("editor.sidebar.connection.connected") : t("editor.sidebar.connection.reconnecting") }}
    </span>
    <button type="button" class="search-btn" id="open-palette" @click="state.palette = true">⌕ {{ t("editor.sidebar.search") }}<kbd>⌘K</kbd></button>
    <div class="label">{{ t("editor.sidebar.screens") }}</div>
    <div id="screens">
      <div v-for="screen in state.inventory.screens" :key="screen.id" class="screen-item" :class="{ selected: screen.id === state.selected && route === '' }">
        <button type="button" class="nav-item" :aria-current="screen.id === state.selected && route === '' ? 'true' : 'false'" @click="select(screen.id)">
          <span class="dot" :class="{ off: !screen.online }"></span>
          <span class="txt">
            <span>{{ screen.name }}</span>
            <small>{{ screen.online ? t("editor.common.online") : t("editor.common.offline") }}{{ screen.area ? " · " + screen.area : "" }} · {{ screen.firmware || t("editor.common.unknown") }}</small>
          </span>
          <span v-if="screen.id === state.selected && state.dirty" class="unsaved" role="img" :aria-label="t('editor.common.unsaved')" :title="t('editor.common.unsaved')"></span>
          <span v-if="updateState(screen)?.kind === 'available'" class="pill">{{ t("editor.sidebar.update.pill") }}</span>
          <span v-else-if="updateState(screen)?.kind === 'running'" class="spin small"></span>
        </button>
        <div v-if="updateState(screen)" class="screen-update" :class="updateState(screen)!.kind">
          <template v-if="updateState(screen)!.kind === 'available'">
            <template v-if="hostFor === screen.id">
              <small>{{ t("editor.sidebar.host.hint") }}</small>
              <form class="screen-host" @submit.prevent="startWithHost(screen)">
                <input v-model="host" :placeholder="t('editor.sidebar.host.placeholder')" required pattern="[A-Za-z0-9][A-Za-z0-9.\-]*" :aria-label="t('editor.sidebar.host.label')" autofocus />
                <button type="submit" class="btn mini primary">{{ t("editor.sidebar.host.start") }}</button>
                <button type="button" class="icon-btn" :aria-label="t('editor.common.cancel')" @click="hostFor = null">✕</button>
              </form>
            </template>
            <template v-else>
              <small>{{ updateState(screen)!.text }}</small>
              <button type="button" class="btn mini primary" :disabled="!screen.update?.profile" :title="screen.update?.profile ? '' : t('editor.sidebar.update.no_profile')" @click="update(screen)">{{ t("editor.sidebar.update.button") }}</button>
            </template>
            <details v-if="notes(screen).length" class="whatsnew">
              <summary>{{ t("editor.sidebar.update.whats_new") }}</summary>
              <ul><li v-for="line in notes(screen).slice(0, 8)" :key="line">{{ line }}</li></ul>
            </details>
          </template>
          <template v-else-if="updateState(screen)!.kind === 'running' && updateProgress(screen)">
            <div class="progress" role="progressbar" :aria-valuenow="updateProgress(screen)!.percent" aria-valuemin="0" aria-valuemax="100"><i :style="{ width: updateProgress(screen)!.percent + '%' }"></i></div>
            <div class="progress-text"><span>{{ updateProgress(screen)!.percent }} %</span><span :title="lastLog()">{{ updateProgress(screen)!.text }}</span></div>
            <small v-if="lastLog()" :title="lastLog()" style="white-space: nowrap; overflow: hidden; text-overflow: ellipsis">{{ lastLog() }}</small>
            <button type="button" class="btn link mini" style="justify-self: start" @click="go('#firmware')">{{ t("editor.sidebar.update.full_log") }}</button>
          </template>
          <small v-else :class="{ failed: updateState(screen)!.kind === 'failed' }">{{ updateState(screen)!.text }}</small>
        </div>
      </div>
    </div>
    <div id="pending">
      <div v-for="p in state.inventory.pending || []" :key="p.file" class="pending">
        <strong>{{ p.friendly }}</strong>
        <small>{{ pendingText(p) }}</small>
        <div class="pending-actions">
          <button type="button" class="btn mini quiet" @click="openIntegrations">{{ t("editor.common.open_integrations") }}</button>
          <button v-if="p.api_key" type="button" class="btn mini quiet" @click="copyText(p.api_key!)">{{ t("editor.sidebar.copy_api_key") }}</button>
        </div>
      </div>
    </div>
    <button id="new-screen" type="button" class="nav-item ghost" :aria-current="route === '#new-screen' ? 'true' : 'false'" @click="go('#new-screen')">
      <span class="plus">+</span><span class="txt">{{ t("editor.nav.new_screen") }}</span>
    </button>
    <div class="spacer"></div>
    <div class="more">
      <div class="label">{{ t("editor.sidebar.more") }}</div>
      <button id="open-firmware" type="button" class="nav-item" :aria-current="route === '#firmware' ? 'true' : 'false'" @click="go('#firmware')"><span class="mdi">{{ glyph("F0241") }}</span><span class="txt">{{ t("editor.nav.firmware") }}</span></button>
      <button id="open-alerts" type="button" class="nav-item" :aria-current="route === '#alerts' ? 'true' : 'false'" @click="go('#alerts')"><span class="mdi">{{ glyph("F0594") }}</span><span class="txt">{{ t("editor.nav.alerts") }}</span></button>
      <button id="open-settings" type="button" class="nav-item" :aria-current="route === '#settings' ? 'true' : 'false'" @click="go('#settings')"><span class="mdi">{{ glyph("F0493") }}</span><span class="txt">{{ t("editor.nav.settings") }}</span></button>
      <button id="refresh" type="button" class="nav-item ghost" @click="refresh()"><span class="plus">↻</span><span class="txt">{{ t("editor.sidebar.refresh") }}</span></button>
    </div>
  </aside>
</template>
