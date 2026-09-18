<script setup lang="ts">
// One tile's settings. Every change applies live, so the card on the mockup shows the result while you pick.
import { computed } from "vue";
import { domainInfo, MAX_PAGES, pageTarget, SLIDER_DOMAINS, TOGGLE_BEFORE } from "../model/layout";
import { glyph } from "../model/topbar";
import { automaticIcon, closeInspector, entityName, markDirty, removeTile, retargetPageTile, setTileOption, state, supports, tileIconCp } from "../store";
import type { Tile } from "../types";
import ActionPicker from "./ActionPicker.vue";
import IconPicker from "./IconPicker.vue";
import Segmented from "./Segmented.vue";

const props = defineProps<{ tile: Tile }>();
const domain = computed(() => props.tile.entity.split(".")[0]);
const name = computed(() => entityName(props.tile.entity));
// A navigation tile (screen.page_<n>): the page it opens, its size, icon and colour; nothing else applies.
const goesTo = computed(() => pageTarget(props.tile.entity));
const pages = Array.from({ length: MAX_PAGES }, (_, i) => [i + 1, String(i + 1)] as [number, string]);
const sizes = computed<[string, string][]>(() => goesTo.value ? [["single", "Normal"], ["wide", "Double-width"]] : [["single", "Normal"], ["wide", "Double-width"], ["full", "Full page"]]);
const sizeHint = computed(() => goesTo.value ? "" : supports(0, 2, 62)
  ? "Full page: one big button that lights up while on; a slider, controls or a graph sit at the bottom of it."
  : "A full-page tile needs firmware 0.2.62: press Update on the screen first.");
const caps = computed(() => state.capabilities[props.tile.entity]);
const current = (key: string, fallback: unknown) => props.tile.options?.[key] ?? fallback;
const display = computed(() => current("display", domain.value === "screen" ? "digital" : "standard") as string);
const displays = computed(() => {
  const list: [string, string][] = domain.value === "screen"
    ? [["digital", "Digital clock"], ["analog", "Analog clock"]]
    : [["standard", "Name and status"], ["watch", "Large value"]];
  const c = caps.value;
  if (domain.value === "weather" && (!c || c.displays.includes("forecast") || display.value === "forecast")) list.push(["forecast", "Weather forecast"]);
  if (domain.value === "sensor" && (!c || c.displays.includes("graph") || display.value === "graph")) list.push(["graph", "Graph"]);
  if (domain.value === "sun") list.push(["sunpath", "Sun path"]);
  return list;
});
const displayHint = computed(() => {
  const c = caps.value;
  if (c && display.value === "graph" && !c.displays.includes("graph")) return "Home Assistant has no numbers for this entity, so the graph stays empty. Choose another display.";
  if (c && display.value === "forecast" && !c.displays.includes("forecast")) return "This weather service has no daily forecast in Home Assistant. Choose another display.";
  return "";
});
const size = computed(() => current("size", "single") as string);
const catalogue = computed(() => state.inventory.controls?.[domain.value]);
const controls = computed(() => current("controls", size.value === "full" ? "none" : catalogue.value?.default) as string);
const controlChoices = computed(() => {
  const c = caps.value;
  return (catalogue.value?.choices || []).filter((ch) => !c || ch.key === "none" || ch.key === controls.value || c.controls.includes(ch.key)).map((ch) => [ch.key, ch.label] as [string, string]);
});
const controlHint = computed(() => {
  const c = caps.value;
  if (c && controls.value !== "none" && !c.controls.includes(controls.value)) return { text: "Home Assistant doesn't offer this control for this entity, so it stays empty on the screen. Choose another one.", warn: true };
  return { text: supports(0, 2, 19)
    ? (size.value === "full" ? "At the bottom of the full-page tile; a tap anywhere else works as set below." : "On the right of the double-width tile, like the rows in Home Assistant. Tapping the name works as set below.")
    : "The screen shows direct control from firmware 0.2.19; until then the tile stays as it was.", warn: false };
});
const tap = computed(() => current("tap", "auto") as string);
const taps = computed(() => {
  const list: [string, string][] = [["auto", "Automatic"], ["detail", "Open control"], ["none", "View only"]];
  // On / off where Home Assistant can toggle the entity, such as a cover; a speaker without on and off gets none.
  if ((caps.value ? caps.value.toggle : TOGGLE_BEFORE.includes(domain.value)) || tap.value === "toggle") list.push(["toggle", "On / off"]);
  list.push(["action", "Perform action"]);
  return list;
});
const tapHint = computed(() => {
  if (tap.value === "toggle" && caps.value && !caps.value.toggle) return { text: "Home Assistant can't turn this on and off, so a tap does nothing. Choose another option.", warn: true };
  if (tap.value === "toggle" && !TOGGLE_BEFORE.includes(domain.value) && !supports(0, 2, 58)) return { text: "The screen switches this from firmware 0.2.58: press Update on the screen. Until then a tap opens its card.", warn: false };
  if (tap.value === "toggle") return { text: "Hold the tile to open its card.", warn: false };
  return null;
});
const inline = computed(() => current("inline", "none") as string);
const showSlider = computed(() => SLIDER_DOMAINS.includes(domain.value) && (!caps.value || caps.value.inline || inline.value === "slider"));
const sliderWarn = computed(() => inline.value === "slider" && caps.value && !caps.value.inline);
const history = computed(() => current("history_hours", 24) as number);
const backgrounds = computed(() => Object.entries(state.inventory.backgrounds || {}));
const fromHA = computed(() => Boolean(state.inventory.entities.find((e) => e.id === props.tile.entity)?.icon));
const showIcon = computed(() => Boolean(state.inventory.icons) && (domain.value !== "screen" || goesTo.value > 0) && !["forecast", "sunpath"].includes(display.value));
function rename(value: string) {
  props.tile.name = value;
  markDirty();
}
function inspect() {
  state.inspector = { kind: "inspect", entity: props.tile.entity };
}
</script>

<template>
  <div class="dr-head">
    <span class="av mdi" :style="{ color: domainInfo(tile.entity)[2], background: domainInfo(tile.entity)[3] }">{{ glyph(tileIconCp(tile)) }}</span>
    <span class="tx"><b>{{ tile.name || name }}</b><small class="mono">{{ tile.entity }}</small></span>
    <button type="button" class="icon-btn" aria-label="Close" @click="closeInspector">✕</button>
  </div>
  <div class="dr-body">
    <div class="f">
      <label class="f-label" for="tile-name">Name on the screen</label>
      <input id="tile-name" :value="tile.name" :placeholder="name" maxlength="60" @input="rename(($event.target as HTMLInputElement).value)" />
    </div>
    <IconPicker v-if="showIcon" :selected="tile.options?.icon || 'auto'" :automatic="automaticIcon(tile.entity)"
      :auto-label="`Automatic (${fromHA ? 'from Home Assistant' : 'default'})`"
      :note="supports(0, 2, 18) ? '' : 'The screen shows a chosen icon from firmware 0.2.18.'"
      @pick="(n) => setTileOption(tile, 'icon', n)" />
    <div v-if="goesTo" class="f">
      <span class="f-label">Goes to page</span>
      <Segmented :choices="pages" :value="goesTo" @pick="(v) => retargetPageTile(tile, Number(v))" />
      <small>{{ supports(0, 2, 62) ? "A tap on the tile opens that page." : "Navigation tiles need firmware 0.2.62: press Update on the screen first." }}</small>
    </div>
    <div v-else class="f">
      <span class="f-label">Display</span>
      <Segmented :choices="displays" :value="display" @pick="(v) => setTileOption(tile, 'display', v)" />
      <small v-if="displayHint" class="warn">{{ displayHint }}</small>
    </div>
    <div class="f">
      <span class="f-label">Size</span>
      <Segmented :choices="sizes" :value="size" @pick="(v) => setTileOption(tile, 'size', v)" />
      <small v-if="sizeHint">{{ sizeHint }}</small>
    </div>
    <div v-if="catalogue && size !== 'single' && !goesTo" class="f">
      <span class="f-label">Direct control on the tile</span>
      <Segmented :choices="controlChoices" :value="controls" @pick="(v) => setTileOption(tile, 'controls', v)" />
      <small :class="{ warn: controlHint.warn }">{{ controlHint.text }}</small>
    </div>
    <div v-if="domain !== 'screen' && !goesTo" class="f">
      <span class="f-label">On tap</span>
      <Segmented :choices="taps" :value="tap" @pick="(v) => setTileOption(tile, 'tap', v)" />
      <small v-if="tapHint" :class="{ warn: tapHint.warn }">{{ tapHint.text }}</small>
    </div>
    <ActionPicker v-if="domain !== 'screen' && !goesTo && tap === 'action'" :tile="tile" />
    <div v-if="showSlider && !goesTo" class="f">
      <span class="f-label">Small slider on the tile</span>
      <Segmented :choices="[['none', 'No'], ['slider', 'Yes, control directly']]" :value="inline" @pick="(v) => setTileOption(tile, 'inline', v)" />
      <small v-if="sliderWarn" class="warn">Home Assistant has nothing a slider can change for this entity. Choose No.</small>
    </div>
    <div v-if="domain === 'sensor' && !goesTo" class="f">
      <span class="f-label">History</span>
      <Segmented :choices="[[1, '1 hour'], [6, '6 hours'], [24, '24 hours']]" :value="history" @pick="(v) => setTileOption(tile, 'history_hours', Number(v))" />
    </div>
    <div class="f">
      <span class="f-label">Pastel background</span>
      <div class="sw">
        <button v-for="[key, choice] in backgrounds" :key="key" type="button" :aria-label="`Background: ${choice.label}`"
          :aria-pressed="(tile.options?.background || 'auto') === key ? 'true' : 'false'" @click="setTileOption(tile, 'background', key)">
          <i :class="choice.color ? '' : key === 'none' ? 'none' : 'auto'" :style="choice.color ? { background: choice.color } : undefined"></i>{{ choice.label }}
        </button>
      </div>
    </div>
  </div>
  <div class="dr-foot">
    <button type="button" class="btn danger" @click="removeTile(tile)">Remove</button>
    <span class="spacer"></span>
    <button v-if="domain !== 'screen'" type="button" class="btn quiet" @click="inspect">Read current data</button>
  </div>
</template>
