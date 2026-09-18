<script setup lang="ts">
// Shared firmware workspace; always a concrete profile and upload target.
import { computed, onBeforeUnmount, onMounted, ref } from "vue";
import { getJson, send } from "../api";
import { go, toast } from "../store";

const ESPHOME_WEB = "https://web.esphome.io/?dashboard_install";
const DOWNLOAD_TARGET = "Download · flash from your own computer";
const data = ref<any>(null);
const file = ref("");
const target = ref("ota");
const host = ref("");
let timer = 0;
async function refreshFirmware(initial = false) {
  try {
    const next = await getJson("firmware");
    data.value = next;
    if (initial || !file.value) file.value = next.profiles[0]?.file || "";
    // USB stays chosen while the board is replugged; a port that went away falls back to the first one.
    const ports: string[] = next.ports || [];
    if (target.value.startsWith("/") && !ports.includes(target.value)) target.value = ports[0] || "usb";
    if (target.value === "usb" && ports.length) target.value = ports[0];
  } catch (e: any) {
    toast(e.message);
  }
}
const running = computed(() => data.value?.job?.state === "running");
const disabled = computed(() => !data.value || running.value || !data.value.available || !data.value.profiles?.length);
const installDisabled = computed(() => disabled.value || target.value === "usb");
const ports = computed<string[]>(() => data.value?.ports || []);
const statusText = computed(() => !data.value ? "Loading…" : !data.value.available
  ? "The ESPHome CLI is missing. Update the app to 0.2.0."
  : data.value.job
    ? `${data.value.job.file} · ${data.value.job.action} · ${data.value.job.state}`
    : "Choose the intended profile, then a USB port, an IP address, or Download.");
const downloadReady = computed(() => target.value === "download" && !!file.value && !!data.value?.downloads?.includes(file.value));
const image = computed(() => ({ href: `api/firmware/profiles/${encodeURIComponent(file.value)}/download`, name: file.value.replace(/\.yaml$/, "") + ".factory.bin" }));
async function run(action: "validate" | "build" | "install") {
  try {
    await send("firmware/jobs", "POST", {
      file: file.value,
      action: action === "validate" ? "validate" : action === "build" ? "build" : target.value === "download" ? "download" : "install",
      target: target.value === "ota" ? host.value.trim() : target.value === "download" ? "" : target.value,
    });
    await refreshFirmware();
  } catch (e: any) {
    toast(e.message);
  }
}
onMounted(() => { refreshFirmware(true); timer = window.setInterval(() => refreshFirmware(), 3000); });
onBeforeUnmount(() => clearInterval(timer));
</script>

<template>
  <div class="panel" id="firmware-dialog">
    <div class="panel-head">
      <div class="tx">
        <span class="eyebrow">Firmware &amp; USB</span>
        <h1>Build and install screen firmware</h1>
        <p>The built-in ESPHome CLI uses your own profile (YAML with keys) and secrets. Choose the screen's profile, then a USB port (screen connected to the Home Assistant machine), <b>Wi-Fi / OTA</b> with the hostname, or <b>Download</b> to put the firmware on the screen from your own computer. <b>Check</b> only validates, <b>Build only</b> compiles, <b>Build &amp; install</b> puts the firmware on the screen. Your tiles stay as they are in ESP Screens. The first build takes a few minutes.</p>
      </div>
      <button type="button" class="btn quiet" id="close-firmware" @click="go('')">← Back</button>
    </div>
    <div class="card">
      <div class="card-grid">
        <div class="field">
          <label class="f-label" for="firmware-file">ESPHome profile</label>
          <select id="firmware-file" v-model="file">
            <option v-for="p in data?.profiles || []" :key="p.file" :value="p.file">{{ p.file }}</option>
          </select>
        </div>
        <div class="field">
          <label class="f-label" for="firmware-port">Install to</label>
          <select id="firmware-port" v-model="target">
            <option value="ota">Wi-Fi / OTA</option>
            <option v-if="!ports.length" value="usb">USB · no board found on the Home Assistant machine yet</option>
            <option v-for="p in ports" :key="p" :value="p">{{ p }}</option>
            <option value="download">{{ DOWNLOAD_TARGET }}</option>
          </select>
        </div>
        <div v-if="target === 'ota'" class="field" id="firmware-host-label">
          <label class="f-label" for="firmware-host">IP address or hostname</label>
          <input id="firmware-host" v-model="host" :placeholder="file ? file.replace(/\.yaml$/, '.local') : 'screen-livingroom.local'" />
        </div>
      </div>
      <div class="actions">
        <button type="button" class="btn quiet" id="firmware-validate" :disabled="disabled" @click="run('validate')">Check</button>
        <button type="button" class="btn quiet" id="firmware-build" :disabled="disabled" @click="run('build')">Build only</button>
        <button type="button" class="btn primary" id="firmware-install" :disabled="installDisabled" @click="run('install')">{{ target === "download" ? "Build & download" : "Build & install" }}</button>
        <span v-if="running" class="spin"></span>
      </div>
      <p id="firmware-status" class="status-line" role="status">{{ statusText }}</p>
      <div v-if="downloadReady" id="firmware-download" class="card" style="background: var(--surface-2)">
        <a class="btn primary" id="firmware-download-link" :href="image.href" :download="image.name">Download {{ image.name }}</a>
        <small>Put it on the screen with <a :href="ESPHOME_WEB" target="_blank" rel="noopener">ESPHome Web</a> in Chrome or Edge: Connect, choose the screen's USB port, Install, and select this file. It holds your Wi-Fi password and the screen's keys: keep it to yourself.</small>
      </div>
    </div>
    <pre id="firmware-log" class="log">{{ (data?.logs || []).join("\n") }}</pre>
  </div>
</template>
