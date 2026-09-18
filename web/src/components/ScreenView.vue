<script setup lang="ts">
// One screen: the head with its status, the Layout and Settings tabs, and the drawer over the right side.
import { computed, onBeforeUnmount, onMounted } from "vue";
import { closeInspector, currentScreen, go, save, state } from "../store";
import LayoutView from "./LayoutView.vue";
import SettingsTab from "./SettingsTab.vue";
import Drawer from "./Drawer.vue";

const screen = computed(() => currentScreen.value!);
const statusText = computed(() => screen.value.online
  ? `${screen.value.delivery} · ${screen.value.status}`
  : "Offline · changes are saved");
const updateReady = computed(() => screen.value.update?.available && screen.value.online && screen.value.update?.profile);
function openOverride() {
  state.menuOpen = false;
  state.overrideProfile = screen.value.update?.profile || null;
  state.overrideFriendly = screen.value.name;
  go("#override");
}
function inspectAll() {
  state.menuOpen = false;
  state.selectedTile = null;
  state.inspector = { kind: "inspect" };
}
function onKey(e: KeyboardEvent) {
  if (e.key === "Escape") {
    if (state.menuOpen) state.menuOpen = false;
    else if (state.inspector && !(e.target as HTMLElement)?.closest?.(".picker")) closeInspector();
  }
}
function onDocClick(e: MouseEvent) {
  if (state.menuOpen && !(e.target as HTMLElement).closest(".head-right .menu, #more")) state.menuOpen = false;
}
onMounted(() => { document.addEventListener("keydown", onKey); document.addEventListener("click", onDocClick); });
onBeforeUnmount(() => { document.removeEventListener("keydown", onKey); document.removeEventListener("click", onDocClick); });
</script>

<template>
  <header class="main-head">
    <div>
      <span class="eyebrow" id="screen-name">{{ screen.name }}</span>
      <h1>{{ state.layout?.title || "Home" }}</h1>
    </div>
    <span id="delivery" class="chip" :class="{ off: !screen.online }" :title="statusText"><span class="dot"></span>{{ statusText }}</span>
    <div class="head-right">
      <div class="seg" role="tablist">
        <button type="button" id="tab-layout" role="tab" :aria-pressed="state.tab === 'layout' ? 'true' : 'false'" @click="state.tab = 'layout'">Layout</button>
        <button type="button" id="tab-settings" role="tab" :aria-pressed="state.tab === 'settings' ? 'true' : 'false'" @click="state.tab = 'settings'; closeInspector()">Screen settings</button>
      </div>
      <span v-if="!state.dirty" id="dirty" class="chip" :class="{ sent: state.saved }">{{ state.saved ? "Saved · sent to screen" : "All saved" }}</span>
      <span v-else id="dirty" class="chip dirty">Unsaved changes</span>
      <button v-if="state.dirty" id="save" type="button" class="btn primary" :disabled="state.busy" @click="save()">
        <span v-if="state.busy" class="spin small"></span>{{ state.busy ? "Saving…" : "Save & send" }}
      </button>
      <button id="more" type="button" class="icon-btn" aria-label="More" aria-haspopup="menu" :aria-expanded="state.menuOpen ? 'true' : 'false'" @click.stop="state.menuOpen = !state.menuOpen">···</button>
      <div v-if="state.menuOpen" class="menu" role="menu">
        <button type="button" role="menuitem" id="inspect" @click="inspectAll">Read current data <small>from Home Assistant</small></button>
        <button type="button" role="menuitem" id="open-override" :disabled="!screen.update?.profile" :title="screen.update?.profile ? '' : 'No ESPHome profile found for this screen'" @click="openOverride">Override YAML <small>advanced</small></button>
        <button type="button" role="menuitem" @click="state.menuOpen = false; go('#firmware')">Firmware &amp; USB</button>
        <button v-if="updateReady" type="button" role="menuitem" @click="state.menuOpen = false; $emit('update')">Update firmware <small>{{ screen.update?.target }}</small></button>
      </div>
    </div>
  </header>
  <div class="body" id="body">
    <LayoutView v-if="state.tab === 'layout'" />
    <SettingsTab v-else />
    <Drawer />
  </div>
</template>
