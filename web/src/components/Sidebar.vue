<script setup lang="ts">
import { ref } from "vue";
import { glyph } from "../model/topbar";
import { copyText, go, openIntegrations, PHASES, refresh, route, select, startUpdate, state } from "../store";
import type { Screen } from "../types";

const hostFor = ref<string | null>(null);
const host = ref("");
function updateState(screen: Screen) {
  const u = screen.update || {};
  if (u.state === "running" || state.updating.includes(screen.id)) return { kind: "running", text: PHASES[u.phase || ""] || "Starting update…" };
  if (u.state === "queued") return { kind: "queued", text: "Queued for the update" };
  if (u.available && screen.online) return { kind: "available", text: `Update ${u.target}` };
  if (u.result && Date.now() / 1000 - u.result.time < 86400) return { kind: u.result.state === "failed" ? "failed" : "done", text: u.result.message };
  return null;
}
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
const pendingText = (p: { installed?: boolean; downloaded?: boolean; file: string }) => p.installed
  ? "Installed, but not yet in Home Assistant. Add the discovered ESPHome device under Settings → Devices & services, paste the API key there, and turn on “Allow the device to perform Home Assistant actions” under Configure."
  : p.downloaded
    ? "Firmware downloaded, not yet in Home Assistant. Put it on the screen with ESPHome Web, then add the discovered ESPHome device under Settings → Devices & services, paste the API key there, and allow the Home Assistant actions under Configure."
    : `Not yet in Home Assistant. Already flashed? Add the ESPHome device under Settings → Devices & services, then allow the Home Assistant actions under Configure. Not flashed yet? Firmware & USB → ${p.file}.`;
</script>

<template>
  <aside class="side">
    <div class="brand"><span class="mark">▦</span><span>ESP Screens</span></div>
    <span id="connection" class="conn" :class="{ online: state.connected }" role="status">
      {{ !state.reachable ? "Management page unreachable · retrying…" : state.connected ? "Home Assistant connected" : "Reconnecting to Home Assistant…" }}
    </span>
    <div class="label">Screens</div>
    <div id="screens">
      <div v-for="screen in state.inventory.screens" :key="screen.id" class="screen-item" :class="{ selected: screen.id === state.selected && route === '' }">
        <button type="button" class="nav-item" :aria-current="screen.id === state.selected && route === '' ? 'true' : 'false'" @click="select(screen.id)">
          <span class="dot" :class="{ off: !screen.online }"></span>
          <span class="txt">
            <span>{{ screen.name }}</span>
            <small>{{ screen.online ? "Online" : "Offline" }}{{ screen.area ? " · " + screen.area : "" }} · {{ screen.firmware || "unknown" }}</small>
          </span>
          <span v-if="updateState(screen)?.kind === 'available'" class="pill">Update</span>
          <span v-else-if="updateState(screen)?.kind === 'running'" class="spin small"></span>
        </button>
        <div v-if="updateState(screen)" class="screen-update" :class="updateState(screen)!.kind">
          <template v-if="updateState(screen)!.kind === 'available'">
            <template v-if="hostFor === screen.id">
              <small>The IP address, once; new firmware reports it itself.</small>
              <form class="screen-host" @submit.prevent="startWithHost(screen)">
                <input v-model="host" placeholder="IP address, e.g. 192.168.1.50" required pattern="[A-Za-z0-9][A-Za-z0-9.\-]*" aria-label="IP address" autofocus />
                <button type="submit" class="btn mini primary">Start</button>
                <button type="button" class="icon-btn" aria-label="Cancel" @click="hostFor = null">✕</button>
              </form>
            </template>
            <template v-else>
              <small>{{ updateState(screen)!.text }}</small>
              <button type="button" class="btn mini primary" :disabled="!screen.update?.profile" :title="screen.update?.profile ? '' : 'No ESPHome profile found with this device name.'" @click="update(screen)">Update</button>
            </template>
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
          <button type="button" class="btn mini quiet" @click="openIntegrations">Open Devices &amp; services</button>
          <button v-if="p.api_key" type="button" class="btn mini quiet" @click="copyText(p.api_key!)">Copy API key</button>
        </div>
      </div>
    </div>
    <button id="new-screen" type="button" class="nav-item ghost" :aria-current="route === '#new-screen' ? 'true' : 'false'" @click="go('#new-screen')">
      <span class="plus">+</span><span class="txt">New screen</span>
    </button>
    <div class="spacer"></div>
    <div class="more">
      <div class="label">More</div>
      <button id="open-firmware" type="button" class="nav-item" :aria-current="route === '#firmware' ? 'true' : 'false'" @click="go('#firmware')"><span class="mdi">{{ glyph("F0241") }}</span><span class="txt">Firmware &amp; USB</span></button>
      <button id="open-alerts" type="button" class="nav-item" :aria-current="route === '#alerts' ? 'true' : 'false'" @click="go('#alerts')"><span class="mdi">{{ glyph("F0594") }}</span><span class="txt">Alerts</span></button>
      <button id="open-settings" type="button" class="nav-item" :aria-current="route === '#settings' ? 'true' : 'false'" @click="go('#settings')"><span class="mdi">{{ glyph("F0493") }}</span><span class="txt">Settings</span></button>
      <button id="refresh" type="button" class="nav-item ghost" @click="refresh()"><span class="plus">↻</span><span class="txt">Refresh</span></button>
    </div>
  </aside>
</template>
