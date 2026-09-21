<script setup lang="ts">
// New screen: profile, Wi-Fi, and the first flash in one go.
import { computed, onBeforeUnmount, onMounted, reactive, ref, watch } from "vue";
import { getJson, send } from "../api";
import { t } from "../i18n";
import { copyText, go, openIntegrations, toast } from "../store";

// Download: ESP Screens builds, the owner flashes the file from their own computer. ESPHome Web is ESPHome's own
// browser flasher; this address opens it with its hint for a downloaded project (as ESPHome Device Builder does).
const ESPHOME_WEB = "https://web.esphome.io/?dashboard_install";
const form = reactive({ board: "cyd", friendly_name: "", name: "", wifi_ssid: "", wifi_password: "", target: "" });
const installer = reactive({
  view: "setup" as "setup" | "progress" | "done", file: null as string | null, friendly: "", board: "cyd", target: "",
  apiKey: null as string | null, nodeEdited: false, jobState: null as string | null, picked: false, action: null as string | null,
});
const nodeVisible = ref(false);
const data = ref<any>(null);
const job = ref<any>(null);
const logs = ref<string[]>([]);
const note = ref("");
const status = ref("");
const submitting = ref(false);
const logOpen = ref(false);
const keyBox = ref<HTMLElement | null>(null);
let poll = 0;

// ESPHome's node-name rule: lowercase ASCII, digits and dashes, starting with a letter.
function slug(text: string) {
  const clean = text.toLowerCase().normalize("NFD").replace(/[̀-ͯ]/g, "")
    .replace(/[^a-z0-9]+/g, "-").replace(/^[^a-z]+/, "").slice(0, 30).replace(/-+$/, "");
  return clean || "screen";
}
function portLabel(port: string) {
  const id = port.replace(/^\/dev\/serial\/by-id\/usb-/, "").replace(/-if\d+(-port\d+)?$/, "").replace(/_/g, " ");
  return `USB · ${id === port ? port.replace(/^\/dev\//, "") : id}`;
}
watch(() => form.friendly_name, () => { if (!installer.nodeEdited) form.name = slug(form.friendly_name); });
const nodePreview = computed(() => form.name || "…");
const ports = computed<string[]>(() => data.value?.ports || []);
const wifi = computed(() => data.value?.wifi);
const askWifi = computed(() => wifi.value?.state === "new" || wifi.value?.state === "missing");
const wifiMissing = computed<string[]>(() => wifi.value?.missing || []);
const wifiStatus = computed(() => t(wifi.value?.state === "new" ? "editor.installer.wifi.new" : "editor.installer.wifi.missing"));
const wifiNote = computed(() => wifi.value?.state === "ready"
  ? t("editor.installer.wifi.ready")
  : wifi.value?.state === "invalid"
    ? t("editor.installer.wifi.invalid")
    : "");
const targetHint = computed(() => t(form.target === "usb"
  ? "editor.installer.target.usb"
  : form.target === "download"
    ? "editor.installer.target.download"
    : !form.target
      ? "editor.installer.target.later"
      : ports.value.length > 1
        ? "editor.installer.target.several"
        : "editor.installer.target.once"));
const goLabel = computed(() => (form.target === "download" ? t("editor.firmware.build_download") : form.target ? t("editor.installer.install") : t("editor.installer.save_profile")));
const busyElsewhere = computed(() => data.value?.job?.state === "running" && !(installer.file && data.value.job.file === installer.file));
const goDisabled = computed(() => submitting.value || wifi.value?.state === "invalid" || form.target === "usb" ||
  (!!form.target && (busyElsewhere.value || !data.value?.available)));
// USB on the Home Assistant machine is always listed first, also before a board is plugged in, so nobody
// concludes it isn't possible; "usb" stands for that port until one shows up.
function syncTarget() {
  const current = form.target;
  const kept = current === "download" || current === "" ? installer.picked : ports.value.includes(current);
  if (!kept) form.target = ports.value[0] || "usb";
}
async function installerRefresh() {
  let next: any;
  try {
    next = await getJson("firmware");
  } catch (e: any) {
    note.value = e.message;
    return;
  }
  data.value = next;
  const current = next.job;
  const ours = current && installer.file && current.file === installer.file;
  if (ours) installer.jobState = current.state;
  if (installer.view === "setup") {
    if (ours && current.state === "running") { showProgress(current, next.logs || []); return; }
    syncTarget();
    note.value = busyElsewhere.value
      ? t("editor.installer.busy", { file: current.file })
      : !next.available && form.target
        ? t("editor.installer.no_cli")
        : wifiNote.value;
  } else if (installer.view === "progress" && ours) {
    job.value = current;
    logs.value = next.logs || [];
    if (current.state !== "running" && current.state !== "success") logOpen.value = true;
  }
}
function showProgress(current: any, lines: string[]) {
  installer.view = "progress";
  installer.jobState = current.state;
  installer.action = current.action;
  job.value = current;
  logs.value = lines;
}
const running = computed(() => job.value?.state === "running");
const ok = computed(() => installer.view === "done" || job.value?.state === "success");
const download = computed(() => installer.action === "download");
const title = computed(() => t(installer.view === "done"
  ? "editor.installer.title.saved"
  : running.value ? "editor.installer.title.running" : ok.value ? (download.value ? "editor.installer.title.ready" : "editor.installer.title.done") : "editor.installer.title.failed"));
const progressTitle = computed(() => installer.view === "done"
  ? t("editor.installer.progress.saved", { file: installer.file })
  : running.value
    ? job.value?.stage === "upload" ? t("editor.installer.progress.writing", { name: installer.friendly }) : t("editor.installer.progress.building")
    : ok.value
      ? download.value ? t("editor.installer.progress.ready", { name: installer.friendly }) : t("editor.installer.progress.installed", { name: installer.friendly })
      : t(download.value ? "editor.installer.progress.build_failed" : "editor.installer.progress.install_failed"));
const progressDetail = computed(() => installer.view === "done"
  ? t("editor.installer.detail.saved")
  : running.value
    ? job.value?.stage === "upload"
      ? t("editor.installer.detail.uploading")
      : t(download.value ? "editor.installer.detail.building_download" : "editor.installer.detail.building")
    : ok.value
      ? download.value
        ? t("editor.installer.detail.downloaded")
        : t(installer.board === "cyd" ? "editor.installer.detail.booted_cyd" : "editor.installer.detail.booted")
      : logs.value.filter((l) => /error/i.test(l)).pop() || logs.value.filter((l) => /failed/i.test(l)).pop() || t("editor.installer.detail.see_log"));
const image = computed(() => ({ href: `api/firmware/profiles/${encodeURIComponent(installer.file || "")}/download`, name: (installer.file || "").replace(/\.yaml$/, "") + ".factory.bin" }));
async function submit(event: Event) {
  const element = event.target as HTMLFormElement;
  if (!installer.nodeEdited) form.name = slug(form.friendly_name);
  if (!element.reportValidity()) return;
  submitting.value = true;
  status.value = "";
  try {
    const payload: Record<string, string> = { board: form.board, friendly_name: form.friendly_name, name: form.name, target: form.target };
    if (askWifi.value) { if (wifiMissing.value.includes("wifi_ssid")) payload.wifi_ssid = form.wifi_ssid; if (wifiMissing.value.includes("wifi_password")) payload.wifi_password = form.wifi_password; }
    const result = await send("firmware/profiles", "POST", payload);
    Object.assign(installer, { file: result.file, apiKey: result.api_key, friendly: form.friendly_name.trim(), board: form.board, target: form.target });
    form.wifi_password = "";
    if (result.job) showProgress(result.job, []);
    else installer.view = "done";
  } catch (err: any) {
    status.value = err.message;
  } finally {
    submitting.value = false;
  }
}
async function retry() {
  try {
    if (!download.value) {
      const { ports: fresh } = await getJson("firmware");
      // The board may have been replugged; a single visible port is unambiguous.
      if (!fresh.includes(installer.target) && fresh.length === 1) installer.target = fresh[0];
    }
    const next = await send("firmware/jobs", "POST", download.value
      ? { file: installer.file, action: "download" }
      : { file: installer.file, action: "install", target: installer.target });
    showProgress(next, []);
  } catch (err: any) {
    toast(err.message);
  }
}
function reset() {
  Object.assign(installer, { view: "setup", file: null, apiKey: null, nodeEdited: false, jobState: null, target: "", picked: false, action: null });
  Object.assign(form, { board: "cyd", friendly_name: "", name: "", wifi_ssid: "", wifi_password: "", target: "" });
  nodeVisible.value = false; job.value = null; logs.value = []; status.value = ""; note.value = ""; logOpen.value = false;
  installerRefresh();
}
function close() {
  if (installer.view === "progress" && installer.jobState !== "running") installer.view = "done";
  go("");
}
onMounted(() => { installerRefresh(); poll = window.setInterval(installerRefresh, 3000); });
onBeforeUnmount(() => clearInterval(poll));
</script>

<template>
  <div class="panel" id="installer">
    <div class="panel-head">
      <div class="tx">
        <span class="eyebrow">{{ t("editor.nav.new_screen") }}</span>
        <h1 id="install-title">{{ installer.view === "setup" ? t("editor.installer.title.setup") : title }}</h1>
        <p v-if="installer.view === 'setup'">{{ t("editor.installer.intro") }}</p>
      </div>
      <button type="button" class="btn quiet" id="close-install" :aria-label="t('editor.common.close')" @click="close">{{ t("editor.common.back") }}</button>
    </div>
    <form v-if="installer.view === 'setup'" id="install-form" class="card" @submit.prevent="submit">
      <fieldset>
        <legend>{{ t("editor.installer.board") }}</legend>
        <div class="boards">
          <label class="board"><input type="radio" name="board" value="cyd" v-model="form.board" /><span><b>{{ t("editor.installer.board_cyd") }}</b><small>ESP32-2432S028 · 320 × 240</small></span></label>
          <label class="board"><input type="radio" name="board" value="guition" v-model="form.board" /><span><b>{{ t("editor.installer.board_guition") }}</b><small>ESP32-S3-4848S040 · 480 × 480 · GT911</small></span></label>
          <label class="board"><input type="radio" name="board" value="waveshare43" v-model="form.board" /><span><b>{{ t("editor.installer.board_waveshare43") }}</b><small>ESP32-S3-Touch-LCD-4.3 · 800 × 480 · GT911</small></span></label>
          <label class="board"><input type="radio" name="board" value="jc8012p4a1" v-model="form.board" /><span><b>{{ t("editor.installer.board_jc8012p4a1") }}</b><small>JC8012P4A1 · 1280 × 800 · GSL3680</small><em>{{ t("editor.installer.board_new") }}</em></span></label>
        </div>
      </fieldset>
      <div class="field">
        <label class="f-label" for="friendly_name">{{ t("editor.installer.name") }}</label>
        <input id="friendly_name" name="friendly_name" v-model="form.friendly_name" required maxlength="60" :placeholder="t('editor.installer.name_placeholder')" autocomplete="off" />
        <small class="node-line">{{ t("editor.installer.device_name") }} <code id="node-preview">{{ nodePreview }}</code><button type="button" class="btn link mini" id="edit-node" @click="installer.nodeEdited = true; nodeVisible = true">{{ t("editor.installer.customize") }}</button></small>
        <small>{{ t("editor.installer.name_hint") }}</small>
      </div>
      <div v-if="nodeVisible" class="field" id="node-label">
        <label class="f-label" for="node-name">{{ t("editor.installer.device_name") }}</label>
        <input id="node-name" name="name" v-model="form.name" pattern="[a-z][a-z0-9\-]{0,29}" maxlength="30" autocomplete="off" @input="installer.nodeEdited = true" />
        <small>{{ t("editor.installer.device_name_hint") }}</small>
      </div>
      <fieldset v-if="askWifi" id="wifi-fields" class="wifi">
        <legend>{{ t("editor.installer.wifi.title") }}</legend>
        <p class="hint" id="wifi-status">{{ wifiStatus }}</p>
        <div v-if="wifiMissing.includes('wifi_ssid')" class="field" id="wifi-ssid-label"><label class="f-label" for="wifi_ssid">{{ t("editor.installer.wifi.ssid") }}</label><input id="wifi_ssid" name="wifi_ssid" v-model="form.wifi_ssid" autocomplete="off" /></div>
        <div v-if="wifiMissing.includes('wifi_password')" class="field" id="wifi-password-label"><label class="f-label" for="wifi_password">{{ t("editor.installer.wifi.password") }}</label><input id="wifi_password" name="wifi_password" type="password" v-model="form.wifi_password" autocomplete="new-password" /></div>
      </fieldset>
      <div class="field">
        <label class="f-label" for="install-target">{{ t("editor.installer.install_via") }}</label>
        <select id="install-target" name="target" v-model="form.target" @change="installer.picked = true; installerRefresh()">
          <option v-if="!ports.length" value="usb">{{ t("editor.firmware.no_board") }}</option>
          <option v-for="p in ports" :key="p" :value="p">{{ portLabel(p) }}</option>
          <option value="download">{{ t("editor.firmware.download_target") }}</option>
          <option value="">{{ t("editor.installer.later") }}</option>
        </select>
        <small id="target-hint">{{ targetHint }}</small>
      </div>
      <p v-if="note" class="hint" id="install-note">{{ note }}</p>
      <div class="actions">
        <button type="submit" class="btn primary" id="install-go" :disabled="goDisabled">{{ goLabel }}</button>
        <span id="install-status" class="status-line error" role="status">{{ status }}</span>
      </div>
    </form>
    <div v-else id="install-progress" class="card">
      <div class="progress-head">
        <span v-if="running" class="spin" id="progress-spin"></span>
        <span v-else class="outcome" :class="ok ? 'ok' : 'bad'" id="progress-mark">{{ ok ? "✓" : "✕" }}</span>
        <strong id="progress-title">{{ progressTitle }}</strong>
      </div>
      <p id="progress-detail">{{ progressDetail }}</p>
      <div v-if="ok && download && installer.view !== 'done'" id="install-download" class="card" style="background: var(--surface-2)">
        <a class="btn primary" id="download-firmware" :href="image.href" :download="image.name">{{ t("editor.firmware.download_file", { name: image.name }) }}</a>
        <ol class="steps" id="download-steps">
          <li><i18n-t keypath="editor.installer.download_steps.plug" scope="global"><template #bold><b>{{ t("editor.installer.download_steps.plug_bold") }}</b></template></i18n-t></li>
          <li><i18n-t keypath="editor.installer.download_steps.open" scope="global">
            <template #bold><b><i18n-t keypath="editor.installer.download_steps.open_bold" scope="global"><template #esphome_web><a :href="ESPHOME_WEB" target="_blank" rel="noopener">ESPHome Web</a></template></i18n-t></b></template>
          </i18n-t></li>
          <li><i18n-t :keypath="installer.board === 'cyd' ? 'editor.installer.download_steps.install_cyd' : 'editor.installer.download_steps.install'" scope="global">
            <template #bold><b>{{ t("editor.installer.download_steps.install_bold") }}</b></template>
            <template #file>{{ image.name }}</template>
          </i18n-t></li>
        </ol>
        <small>{{ t("editor.installer.download_keep") }}</small>
      </div>
      <div v-if="ok" id="install-result" class="field">
        <div class="key-box">
          <span>{{ t("editor.installer.api_key") }}</span><code id="api-key" ref="keyBox">{{ installer.apiKey || "" }}</code>
          <button type="button" class="btn quiet mini" id="copy-key" @click="copyText(installer.apiKey || '', keyBox)">{{ t("editor.common.copy") }}</button>
        </div>
        <ol class="steps" id="install-steps">
          <li><i18n-t keypath="editor.installer.pairing.ha" scope="global"><template #bold><b>{{ t("editor.installer.pairing.ha_bold") }}</b></template><template #name>{{ installer.friendly }}</template></i18n-t> <button type="button" class="btn quiet mini" @click="openIntegrations">{{ t("editor.common.open_integrations") }}</button></li>
          <li><i18n-t keypath="editor.installer.pairing.key" scope="global"><template #bold><b>{{ t("editor.installer.pairing.key_bold") }}</b></template></i18n-t></li>
          <li><i18n-t keypath="editor.installer.pairing.actions" scope="global"><template #bold><b>{{ t("editor.installer.pairing.actions_bold") }}</b></template></i18n-t></li>
          <li><i18n-t keypath="editor.installer.pairing.tiles" scope="global"><template #bold><b>{{ t("editor.installer.pairing.tiles_bold") }}</b></template></i18n-t></li>
        </ol>
      </div>
      <details v-if="installer.view !== 'done'" id="install-log-wrap" class="log-wrap" :open="logOpen" @toggle="logOpen = ($event.target as HTMLDetailsElement).open">
        <summary>{{ t("editor.installer.log") }}</summary>
        <pre id="install-log" class="log">{{ logs.join("\n") }}</pre>
      </details>
      <div class="actions">
        <button v-if="!running && !ok" type="button" class="btn primary" id="install-retry" @click="retry">{{ t("editor.installer.retry") }}</button>
        <button type="button" class="btn quiet" id="install-close" @click="reset">{{ ok ? t("editor.installer.another") : t("editor.installer.start_over") }}</button>
        <button type="button" class="btn" :class="ok ? 'primary' : 'quiet'" @click="close">{{ ok ? t("editor.installer.done") : t("editor.common.close") }}</button>
      </div>
    </div>
  </div>
</template>
