<script setup lang="ts">
// A card on the mockup, drawn with what Home Assistant reports right now. A placeholder is the tile being
// dragged, drawn where it will land.
import { computed, nextTick } from "vue";
import { vDrag } from "../drag";
import { displayNames, effectiveControls, isWide, pageOf, SLOTS_PER_PAGE } from "../model/layout";
import { glyph } from "../model/topbar";
import { entityName, liveOf, openTile, placeTile, removeTile, state, tileIconCp } from "../store";
import type { Tile } from "../types";

const props = defineProps<{ tile: Tile; slot: number; placeholder?: boolean }>();
const name = computed(() => props.tile.name || entityName(props.tile.entity));
const wide = computed(() => isWide(props.tile));
const background = computed(() => state.inventory.backgrounds?.[props.tile.options?.background || ""]?.color);
const bare = computed(() => props.tile.options?.background === "none");
const display = computed(() => props.tile.options?.display || "standard");
const note = computed(() => (display.value !== "standard" ? displayNames[display.value] || display.value : ""));
const controls = computed(() => effectiveControls(props.tile, state.inventory));
const domain = computed(() => props.tile.entity.split(".")[0]);
const cp = computed(() => state.inventory.icons?.controls || {});
const key = (n: string) => (cp.value[n] ? glyph(cp.value[n]) : "");
const chosen = computed(() => state.selectedTile === props.tile.entity && state.inspector?.kind === "tile");
const live = computed(() => !props.placeholder && state.layout?.tiles.includes(props.tile));
const label = computed(() => `${name.value}, slot ${(props.slot % SLOTS_PER_PAGE) + 1} on page ${pageOf(props.slot) + 1}. Enter: configure, arrow keys: move`);
const now = computed(() => new Date(state.now));
const hourAngle = computed(() => (now.value.getHours() % 12 + now.value.getMinutes() / 60) * 30);
const minuteAngle = computed(() => now.value.getMinutes() * 6);

// ---- Live values ----
const current = computed(() => (domain.value === "screen" ? null : liveOf(props.tile.entity)));
const gone = computed(() => !current.value || ["unavailable", "unknown", ""].includes(current.value.state));
const on = computed(() => Boolean(current.value) && !gone.value && current.value!.state !== "off" && current.value!.state !== "closed" && current.value!.state !== "standby" && current.value!.state !== "idle" && current.value!.state !== "docked");
const isOn = computed(() => ["light", "switch", "input_boolean", "fan"].includes(domain.value) && current.value?.state === "on");
const unit = computed(() => current.value?.a?.unit_of_measurement as string | undefined);
const capital = (text: string) => text.charAt(0).toUpperCase() + text.slice(1).replace(/_/g, " ");
// Home Assistant's words for the weather states, for a Home Assistant that hands us none.
const WEATHER_WORDS: Record<string, string> = { "clear-night": "Clear, night", cloudy: "Cloudy", exceptional: "Exceptional", fog: "Fog", hail: "Hail", lightning: "Lightning", "lightning-rainy": "Lightning, rainy", partlycloudy: "Partly cloudy", pouring: "Pouring", rainy: "Rainy", snowy: "Snowy", "snowy-rainy": "Snowy, rainy", sunny: "Sunny", windy: "Windy", "windy-variant": "Windy" };
// A scene, script or button has no state worth a word: its state is the moment it last ran.
const NO_STATUS = ["scene", "script", "button", "input_button"];
// The text under the name: Home Assistant's word where it has one, the value with its unit for a sensor.
const status = computed(() => {
  const c = current.value;
  if (!c || NO_STATUS.includes(domain.value)) return note.value;
  if (gone.value) return c.state === "unknown" ? "Unknown" : "Unavailable";
  const a = c.a || {};
  if (domain.value === "climate") return `${a.current_temperature !== undefined ? `${a.current_temperature}° · ` : ""}${c.word || capital(c.state)}`;
  if (domain.value === "weather") return `${c.word || WEATHER_WORDS[c.state] || capital(c.state)}${a.temperature !== undefined ? ` · ${a.temperature}°` : ""}`;
  if (domain.value === "cover" && a.current_position !== undefined && a.current_position > 0 && a.current_position < 100) return `${c.word || capital(c.state)} · ${a.current_position} %`;
  if (domain.value === "media_player" && a.media_title) return `${c.word || capital(c.state)} · ${a.media_title}`;
  if (domain.value === "sensor") return `${c.state}${unit.value ? ` ${unit.value}` : ""}`;
  if (domain.value === "timer") return c.word || capital(c.state);
  return c.word || capital(c.state);
});
const bigValue = computed(() => (current.value && !gone.value ? current.value.state : "—"));
// The small slider's fill, from what the entity reports; off is empty, like the screen's grey fill.
const fill = computed(() => {
  const c = current.value;
  if (!c || gone.value) return 0;
  const a = c.a || {};
  if (domain.value === "light") return c.state === "on" ? (a.brightness !== undefined ? Math.round((a.brightness / 255) * 100) : 100) : 0;
  if (domain.value === "fan") return c.state === "on" ? (a.percentage ?? 100) : 0;
  if (domain.value === "cover") return a.current_position ?? (c.state === "open" ? 100 : 0);
  if (domain.value === "media_player") return Math.round((a.volume_level ?? 0) * 100);
  if (domain.value === "number" || domain.value === "input_number") {
    const value = Number(c.state), min = Number(a.min ?? 0), max = Number(a.max ?? 100);
    return Number.isFinite(value) && max > min ? Math.round(((value - min) / (max - min)) * 100) : 0;
  }
  return 0;
});
const sliderStyle = computed(() => ({ background: `linear-gradient(to right, ${fill.value ? "#ffbf38" : "#c9ccd1"} ${fill.value}%, ${fill.value ? "#fff1d3" : "#e6e8ec"} ${fill.value}%)` }));
const volumeStyle = computed(() => ({ background: `linear-gradient(to right, #2196f3 ${fill.value}%, #d3e8fb ${fill.value}%)` }));
const setpoint = computed(() => {
  const t = current.value?.a?.temperature;
  return t !== undefined && t !== null ? `${t}°` : "—";
});

async function onKey(e: KeyboardEvent) {
  if (e.key === "Enter" || e.key === " ") { e.preventDefault(); openTile(props.tile.entity); return; }
  const step = ({ ArrowLeft: -1, ArrowRight: 1, ArrowUp: -2, ArrowDown: 2 } as Record<string, number>)[e.key];
  if (!step) return;
  e.preventDefault();
  // A wide card owns its row: left and right mean the row above and below.
  if (placeTile(props.tile, props.tile.slot + (wide.value ? Math.sign(step) * 2 : step))) {
    await nextTick();
    document.querySelector<HTMLElement>(`.pages [data-slot="${props.tile.slot}"]`)?.focus();
  }
}
</script>

<template>
  <div class="tile" :class="{ wide, bare, placeholder: placeholder || !live, chosen }" :data-slot="slot"
    :style="background && !bare ? { backgroundColor: background } : undefined"
    :tabindex="live ? 0 : -1" :role="live ? 'button' : undefined" :aria-label="live ? label : undefined"
    v-drag="{ kind: 'tile', tile }" @click="live && openTile(tile.entity)" @keydown="live && onKey($event)">
    <template v-if="display === 'analog'">
      <svg class="clockface" viewBox="0 0 60 60" aria-hidden="true">
        <circle cx="30" cy="30" r="27" fill="#fff" stroke="#c9ccd1" />
        <line v-for="a in [0, 90, 180, 270]" :key="a" x1="30" y1="5" x2="30" y2="9" stroke="#1b1b1b" stroke-width="1.5" :transform="`rotate(${a} 30 30)`" />
        <line x1="30" y1="30" x2="30" y2="16" stroke="#1b1b1b" stroke-width="2.4" stroke-linecap="round" :transform="`rotate(${hourAngle} 30 30)`" />
        <line x1="30" y1="30" x2="30" y2="11" stroke="#1b1b1b" stroke-width="1.6" stroke-linecap="round" :transform="`rotate(${minuteAngle} 30 30)`" />
        <circle cx="30" cy="30" r="1.8" fill="#1b1b1b" />
      </svg>
      <span v-if="wide" class="lead"><span class="tx"><span class="nm">{{ name }}</span><span class="st">{{ note }}</span></span></span>
    </template>
    <template v-else-if="display === 'digital' && domain === 'screen'">
      <span class="ic mdi">{{ glyph(tileIconCp(tile)) }}</span>
      <span class="lead"><span class="big">{{ String(now.getHours()).padStart(2, "0") }}:{{ String(now.getMinutes()).padStart(2, "0") }}</span><span class="nm">{{ name }}</span></span>
    </template>
    <template v-else-if="wide">
      <span class="lead">
        <span class="ic mdi" :class="{ lit: isOn }">{{ glyph(tileIconCp(tile)) }}</span>
        <span class="tx">
          <span class="nm">{{ name }}</span>
          <span v-if="display === 'watch'" class="big">{{ bigValue }}<small v-if="unit && !gone">{{ unit }}</small></span>
          <span v-else-if="status" class="st" :class="{ off: gone }">{{ status }}</span>
        </span>
      </span>
      <span v-if="tile.options?.inline === 'slider'" class="mini-slider" :style="sliderStyle"></span>
      <span v-if="controls" class="ctl">
        <span v-if="controls === 'toggle'" class="tog" :class="{ off: !on }"></span>
        <span v-else-if="controls === 'setpoint'" class="stp"><span class="mdi">{{ key("minus") || "−" }}</span><b>{{ setpoint }}</b><span class="mdi">{{ key("plus") || "+" }}</span></span>
        <template v-else-if="controls === 'stepper' && domain.endsWith('select')"><span class="key mdi">{{ key("chevron-left") }}</span><span class="key mdi">{{ key("chevron-right") }}</span></template>
        <span v-else-if="controls === 'stepper'" class="stp"><span class="mdi">{{ key("minus") || "−" }}</span><b>{{ bigValue }}</b><span class="mdi">{{ key("plus") || "+" }}</span></span>
        <template v-else-if="controls === 'mode'"><span class="key mdi">{{ key("power") }}</span><span class="key mdi">{{ key("fire") }}</span><span class="key mdi">{{ key("snowflake") }}</span></template>
        <template v-else-if="controls === 'volume'"><span class="range" :style="volumeStyle"></span><span class="key mdi">{{ key("volume-high") }}</span></template>
        <template v-else-if="controls === 'playback'"><span class="key mdi">{{ key("skip-previous") }}</span><span class="key mdi">{{ key(on ? "pause" : "play") || key("play") }}</span><span class="key mdi">{{ key("skip-next") }}</span></template>
        <template v-else-if="controls === 'buttons' && domain === 'cover'"><span class="key mdi">{{ key("arrow-expand-horizontal") }}</span><span class="key mdi">{{ key("stop") }}</span><span class="key mdi">{{ key("arrow-collapse-horizontal") }}</span></template>
        <template v-else-if="controls === 'buttons' && domain === 'vacuum'"><span class="key mdi">{{ key("play") }}</span><span class="key mdi">{{ key("stop") }}</span><span class="key mdi">{{ key("home-map-marker") }}</span></template>
        <template v-else-if="controls === 'buttons' && domain === 'timer'"><span class="key mdi">{{ key("play") }}</span><span class="key mdi">{{ key("close") }}</span></template>
        <span v-else-if="controls === 'run'" class="run">{{ ({ scene: "Activate", script: "Run" } as Record<string, string>)[domain] || "Press" }}</span>
        <span v-else class="range" :style="{ background: `linear-gradient(to right, #2196f3 ${fill}%, #d3e8fb ${fill}%)` }"></span>
      </span>
    </template>
    <template v-else>
      <span class="ic mdi" :class="{ lit: isOn }">{{ glyph(tileIconCp(tile)) }}</span>
      <span class="lead">
        <span v-if="display === 'watch'" class="big">{{ bigValue }}<small v-if="unit && !gone">{{ unit }}</small></span>
        <span class="nm">{{ name }}</span>
        <span v-if="display !== 'watch' && status" class="st" :class="{ off: gone }">{{ status }}</span>
        <span v-if="tile.options?.inline === 'slider'" class="mini-slider" :style="sliderStyle"></span>
      </span>
    </template>
    <button v-if="live" type="button" class="remove" title="Remove tile" :aria-label="`Remove ${name}`" @click.stop="removeTile(tile)">✕</button>
  </div>
</template>
