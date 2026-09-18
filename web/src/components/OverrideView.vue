<script setup lang="ts">
// Per-screen local YAML override: a small file of the owner's, loaded after the shared screen package.
import { computed, onBeforeUnmount, onMounted, ref } from "vue";
import { getJson, send } from "../api";
import { glyph } from "../model/topbar";
import { go, state, toast } from "../store";

const OVERRIDE_EXAMPLE = `# Hardware-specific changes for this screen.
# This file is kept when the shared firmware package updates.
# Do not add esphome:, api:, ota:, wifi: or packages: here.

# Example: a CYD with the ST7789V display controller.
# "!extend" changes the display the shared package already defines;
# a bare id would add a second, incomplete display and fail the build.
display:
  - id: !extend my_display
    model: ST7789V
`;
const profile = computed(() => state.overrideProfile);
const content = ref("");
const file = ref("");
const attached = ref(false);
const status = ref("Loading…");
const kind = ref("");
const busy = ref(false);
const editor = ref<HTMLTextAreaElement | null>(null);
const gutter = ref<HTMLDivElement | null>(null);
const lines = computed(() => content.value.split("\n").length);
const gutterText = computed(() => Array.from({ length: lines.value }, (_, i) => i + 1).join("\n"));
const count = computed(() => `${content.value.length} characters · ${lines.value} line${lines.value === 1 ? "" : "s"}`);
let alive = true;
function setStatus(message: string, k = "") { status.value = message; kind.value = k; }
async function load() {
  if (!profile.value) { setStatus("No ESPHome profile was found for this screen.", "error"); return; }
  setStatus("Loading…");
  try {
    const data = await getJson(`firmware/profiles/${encodeURIComponent(profile.value)}/override`);
    file.value = data.override_file;
    attached.value = Boolean(data.attached);
    content.value = data.exists && data.content !== "{}\n" ? data.content : "";
    setStatus(data.attached ? "Changes here apply on the next build." : "Save once to attach this file to the profile.");
  } catch (error: any) {
    setStatus(error.message, "error");
  }
}
async function saveOverride(runCheck = false) {
  if (!profile.value || busy.value) return;
  busy.value = true;
  setStatus("Checking and saving…");
  try {
    const data = await send(`firmware/profiles/${encodeURIComponent(profile.value)}/override`, "PUT", { content: content.value });
    file.value = data.override_file;
    attached.value = true;
    if (!runCheck) {
      setStatus("Saved. The override is kept during firmware updates.", "ok");
      toast("Override YAML saved.");
      return;
    }
    setStatus("Saved. ESPHome is checking the complete profile…");
    await send("firmware/jobs", "POST", { file: profile.value, action: "validate" });
    pollCheck();
  } catch (error: any) {
    setStatus(error.message, "error");
  } finally {
    busy.value = false;
  }
}
function pollCheck() {
  const started = Date.now(), checked = profile.value;
  const poll = async () => {
    if (!alive || profile.value !== checked) return;  // the page closed or shows another screen
    try {
      const data = await getJson("firmware");
      const current = data.job;
      if (current && current.file === checked && current.state === "running") {
        setStatus(`ESPHome is checking the profile… ${current.stage || ""}`.trim());
      } else if (current && current.file === checked && current.state === "success") {
        setStatus("The complete profile is valid. Safe to build and install.", "ok");
        return;
      } else if (current && current.file === checked && current.state === "failed") {
        const error = (data.logs || []).filter((line: string) => /error|failed/i.test(line)).pop();
        setStatus(error || "ESPHome rejected the complete profile. See Firmware & USB for the full log.", "error");
        return;
      }
      if (Date.now() - started < 7200000) setTimeout(poll, 1200);
    } catch (error: any) {
      setStatus(error.message, "error");
    }
  };
  poll();
}
function useExample() {
  if (!content.value.trim() || confirm("Replace the current text with the example?")) {
    content.value = OVERRIDE_EXAMPLE;
    setStatus("Example loaded. Save it when you are ready.");
    editor.value?.focus();
  }
}
function clear() {
  if (!content.value.trim() || confirm("Clear the local override?")) {
    content.value = "";
    setStatus("The override will be empty after you save.");
    editor.value?.focus();
  }
}
function onKey(event: KeyboardEvent) {
  const area = editor.value!;
  if (event.key === "Tab") {
    event.preventDefault();
    const start = area.selectionStart, end = area.selectionEnd;
    area.setRangeText("  ", start, end, "end");
    content.value = area.value;
  } else if ((event.ctrlKey || event.metaKey) && event.key.toLowerCase() === "s") {
    event.preventDefault();
    saveOverride(false);
  }
}
function syncScroll() { if (gutter.value && editor.value) gutter.value.scrollTop = editor.value.scrollTop; }
onMounted(load);
onBeforeUnmount(() => { alive = false; });
</script>

<template>
  <div class="panel" id="override-dialog">
    <div class="panel-head">
      <div class="tx">
        <span class="eyebrow">Advanced · local override</span>
        <h1 id="override-title">Override YAML{{ state.overrideFriendly ? ` · ${state.overrideFriendly}` : "" }}</h1>
        <p>This small file is yours. It is loaded after the shared screen package and stays in place when ESP Screen Manager updates. Change hardware-specific details here, such as a display model.</p>
      </div>
      <button type="button" class="btn quiet" id="close-override" @click="go('')">← Back</button>
    </div>
    <div class="notice">
      <span class="mdi">{{ glyph("F0493") }}</span>
      <div><strong>What is safe to change?</strong><span>Display, touchscreen, substitutions, pins and other device details. Name, Wi-Fi, API, OTA and package links remain managed.</span></div>
    </div>
    <div class="file-row">
      <code id="override-file">{{ file || profile || "" }}</code>
      <span id="override-state" class="chip" :class="{ good: attached }">{{ attached ? "Active" : "Ready to attach" }}</span>
    </div>
    <div class="yaml-editor" id="yaml-editor-wrap">
      <div ref="gutter" class="yaml-gutter" id="override-gutter" aria-hidden="true">{{ gutterText }}</div>
      <textarea ref="editor" id="override-editor" v-model="content" spellcheck="false" autocapitalize="off" autocomplete="off" autocorrect="off"
        aria-label="Local override YAML" placeholder="# Example&#10;display:&#10;  - id: !extend my_display&#10;    model: ST7789V" @keydown="onKey" @scroll="syncScroll"></textarea>
    </div>
    <div class="actions">
      <button type="button" class="btn quiet mini" id="override-example" @click="useExample">Use example</button>
      <button type="button" class="btn quiet mini" id="override-empty" @click="clear">Clear</button>
      <span id="override-count" class="hint">{{ count }}</span>
    </div>
    <p id="override-status" class="status-line" :class="kind" role="status">{{ status }}</p>
    <div class="actions">
      <button type="button" class="btn quiet" id="override-check" :disabled="busy || !profile" @click="saveOverride(true)">Save &amp; check</button>
      <button type="button" class="btn primary" id="override-save" :disabled="busy || !profile" @click="saveOverride(false)">Save override</button>
    </div>
  </div>
</template>
