<script setup lang="ts">
// Everything around the screens: firmware updates, alerts, Claude.
import { computed, ref } from "vue";
import { anyUpdating, go, installClaudeSkill, runUpdateAll, setAutoUpdate, state, updateProgress } from "../store";

const u = computed(() => state.inventory.updates);
const outdated = computed(() => state.inventory.screens.filter((s) => s.update?.available).length);
const updatesHint = computed(() => !u.value ? "" : u.value.busy
  ? `Updating to firmware ${u.value.target}…`
  : !state.inventory.screens.length
    ? `No screen paired yet. New screens get firmware ${u.value.target}.`
    : outdated.value
      ? `Firmware ${u.value.target} is available for ${outdated.value} screen${outdated.value === 1 ? "" : "s"}.`
      : `All screens have firmware ${u.value.target}.`);
const running = computed(() => state.inventory.screens.find((s) => s.update?.state === "running" || state.updating.includes(s.id)));
const progress = computed(() => (running.value ? updateProgress(running.value) : null));
const logTail = computed(() => (state.firmwareJob?.logs || []).slice(-12).join("\n"));
// What the current firmware brings: the changelog sections that mention it.
const targetNotes = computed(() => {
  const sections = state.inventory.changelog, target = u.value?.target;
  if (!Array.isArray(sections) || !target) return [];
  return sections.filter((s) => s.firmware === target).flatMap((s) => s.lines).slice(0, 10);
});
const skill = computed(() => state.inventory.claude_skill);
const installing = ref(false);
async function install() {
  installing.value = true;
  try { await installClaudeSkill(); } finally { installing.value = false; }
}
</script>

<template>
  <div class="panel" id="settings-view">
    <div class="panel-head">
      <div class="tx">
        <span class="eyebrow">Settings</span>
        <h1 id="settings-title">Everything around your screens.</h1>
      </div>
      <button type="button" class="btn quiet" id="close-settings" @click="go('')">← My screens</button>
    </div>
    <div class="card-grid">
      <section class="card">
        <h2>Screens</h2>
        <p>Add a screen, or build and reinstall the firmware of one you already have.</p>
        <div class="tools">
          <button type="button" class="tool" @click="go('#new-screen')"><span class="tool-icon">＋</span><span class="tx"><b>New screen</b><small>Connect and install</small></span></button>
          <button type="button" class="tool" @click="go('#firmware')"><span class="tool-icon">⇪</span><span class="tx"><b>Firmware &amp; USB</b><small>Build and install via USB or Wi-Fi</small></span></button>
        </div>
      </section>
      <section v-if="u" class="card updates" id="updates">
        <h2>Firmware updates</h2>
        <p id="updates-hint">{{ updatesHint }}</p>
        <template v-if="running && progress">
          <div class="progress" role="progressbar" :aria-valuenow="progress.percent" aria-valuemin="0" aria-valuemax="100"><i :style="{ width: progress.percent + '%' }"></i></div>
          <div class="progress-text"><span>{{ running.name }} · {{ progress.percent }} %</span><span>{{ progress.text }}</span></div>
          <pre v-if="logTail" class="log-lines">{{ logTail }}</pre>
        </template>
        <button v-if="u.pending && !u.busy && u.pending >= 2" id="update-all" type="button" class="btn primary" @click="runUpdateAll">Update all {{ u.pending }} screens</button>
        <details v-if="targetNotes.length && !anyUpdating()" class="whatsnew">
          <summary>What's new in firmware {{ u.target }}</summary>
          <ul><li v-for="line in targetNotes" :key="line">{{ line }}</li></ul>
        </details>
        <label class="check">
          <input type="checkbox" id="auto-update" :checked="Boolean(u.auto)" @change="setAutoUpdate(($event.target as HTMLInputElement).checked)" />
          <span>Update automatically every night<small>Between 03:00 and 06:00, one screen at a time. Stops on an error and reports it in Home Assistant.</small></span>
        </label>
      </section>
      <section class="card">
        <h2>Alerts</h2>
        <p>A card over the whole screen from an automation, on one screen or on all of them. The cheatsheet has the action names, fields, icons, and colors, and a form to try one.</p>
        <div class="tools">
          <button type="button" class="tool" @click="go('#alerts')"><span class="tool-icon">!</span><span class="tx"><b>Alerts</b><small>Full-screen notifications, from an automation</small></span></button>
        </div>
      </section>
      <section class="card" id="claude">
        <h2>Claude</h2>
        <p>Teach Claude about ESP Screens, then ask: "Show an alert on all my screens when the mailbox is full." Claude knows the alert events, every field, color, and icon.</p>
        <div class="actions">
          <button type="button" id="claude-install" class="btn" :class="skill?.installed && skill.current ? 'quiet' : 'primary'" :disabled="!skill || installing" @click="install">
            {{ !skill || !skill.installed ? "Install for Claude Code" : skill.current ? "Install again" : "Update the skill" }}
          </button>
          <span id="claude-status" class="chip" :class="!skill ? '' : !skill.installed ? '' : skill.current ? 'good' : 'update'" role="status">
            {{ !skill ? "Loading…" : !skill.installed ? "Not installed" : skill.current ? "● Installed" : "Installed · a newer version is ready" }}
          </span>
        </div>
        <small>For Claude Code in Home Assistant. Writes one file to <code id="claude-path">{{ skill?.path || "/homeassistant/.claude/skills/esp-screens" }}</code>; installing again replaces it.</small>
        <div class="actions">
          <a class="btn quiet" id="claude-download" href="api/claude-skill.zip" download="esp-screens.zip">Download for claude.ai</a>
          <small>Upload the zip in Claude under Customize → Skills.</small>
        </div>
      </section>
    </div>
  </div>
</template>
