<script setup lang="ts">
// One screen: the head with its status, the Layout and Settings tabs, and the drawer over the right side.
import { computed, onBeforeUnmount, onMounted, ref } from "vue";
import { canAlert, closeInspector, copyLayoutFrom, currentScreen, exportLayout, go, identify, importLayout, save, startUpdate, state } from "../store";
import LayoutView from "./LayoutView.vue";
import SettingsTab from "./SettingsTab.vue";
import Drawer from "./Drawer.vue";

const screen = computed(() => currentScreen.value!);
const statusText = computed(() => screen.value.online
  ? `${screen.value.delivery} · ${screen.value.status}`
  : "Offline · changes are saved");
const updateReady = computed(() => screen.value.update?.available && screen.value.online && screen.value.update?.profile && screen.value.update?.host);
const others = computed(() => state.inventory.screens.filter((s) => s.id !== screen.value.id && s.layout?.tiles?.length));
const copyOpen = ref(false);
const fileInput = ref<HTMLInputElement | null>(null);
function closeMenu() { state.menuOpen = false; copyOpen.value = false; }
function openOverride() {
  closeMenu();
  state.overrideProfile = screen.value.update?.profile || null;
  state.overrideFriendly = screen.value.name;
  go("#override");
}
function inspectAll() {
  closeMenu();
  state.selectedTile = null;
  state.inspector = { kind: "inspect" };
}
function copyFrom(id: string) {
  closeMenu();
  if (state.dirty && !confirm("Replace the unsaved layout with a copy of the other screen's?")) return;
  copyLayoutFrom(id);
}
function pickFile() { closeMenu(); fileInput.value?.click(); }
async function onFile(e: Event) {
  const input = e.target as HTMLInputElement;
  const file = input.files?.[0];
  input.value = "";
  if (!file) return;
  if (state.dirty && !confirm("Replace the unsaved layout with the imported one?")) return;
  importLayout(await file.text());
}
function onKey(e: KeyboardEvent) {
  if (e.key === "Escape") {
    if (state.menuOpen) closeMenu();
    else if (state.palette) return;
    else if (state.inspector && !(e.target as HTMLElement)?.closest?.(".picker")) closeInspector();
  } else if ((e.metaKey || e.ctrlKey) && e.key.toLowerCase() === "s") {
    e.preventDefault();
    if (state.dirty) save();
  }
}
function onDocClick(e: MouseEvent) {
  if (state.menuOpen && !(e.target as HTMLElement).closest(".head-right .menu, #more")) closeMenu();
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
      <button v-if="state.dirty" id="save" type="button" class="btn primary" :disabled="state.busy" title="⌘S" @click="save()">
        <span v-if="state.busy" class="spin small"></span>{{ state.busy ? "Saving…" : "Save & send" }}
      </button>
      <button id="more" type="button" class="icon-btn" aria-label="More" aria-haspopup="menu" :aria-expanded="state.menuOpen ? 'true' : 'false'" @click.stop="state.menuOpen = !state.menuOpen; copyOpen = false">···</button>
      <div v-if="state.menuOpen" class="menu" role="menu">
        <button type="button" role="menuitem" id="identify" :disabled="!canAlert(screen) || !screen.online" :title="canAlert(screen) ? '' : 'Needs firmware 0.2.31 or newer'" @click="closeMenu(); identify(screen)">Identify <small>blink the screen</small></button>
        <button type="button" role="menuitem" id="inspect" @click="inspectAll">Read current data <small>from Home Assistant</small></button>
        <div class="sep"></div>
        <button type="button" role="menuitem" id="copy-layout" :disabled="!others.length" :aria-expanded="copyOpen ? 'true' : 'false'" @click.stop="copyOpen = !copyOpen">Copy layout from… <small>{{ others.length ? `${others.length} screen${others.length === 1 ? "" : "s"}` : "no other screen" }}</small></button>
        <div v-if="copyOpen" class="sub">
          <button v-for="other in others" :key="other.id" type="button" role="menuitem" @click="copyFrom(other.id)">{{ other.name }} <small>{{ other.layout.tiles.length }} tiles</small></button>
        </div>
        <button type="button" role="menuitem" id="export-layout" @click="closeMenu(); exportLayout()">Export layout <small>JSON</small></button>
        <button type="button" role="menuitem" id="import-layout" @click="pickFile">Import layout… <small>JSON</small></button>
        <div class="sep"></div>
        <button type="button" role="menuitem" id="open-override" :disabled="!screen.update?.profile" :title="screen.update?.profile ? '' : 'No ESPHome profile found for this screen'" @click="openOverride">Override YAML <small>advanced</small></button>
        <button type="button" role="menuitem" @click="closeMenu(); go('#firmware')">Firmware &amp; USB</button>
        <button v-if="updateReady" type="button" role="menuitem" @click="closeMenu(); startUpdate(screen)">Update firmware <small>{{ screen.update?.target }}</small></button>
      </div>
      <input ref="fileInput" type="file" accept="application/json,.json" hidden @change="onFile" />
    </div>
  </header>
  <div class="body" id="body">
    <LayoutView v-if="state.tab === 'layout'" />
    <SettingsTab v-else />
    <Drawer />
  </div>
</template>
