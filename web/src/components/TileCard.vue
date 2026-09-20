<script setup lang="ts">
// A card on the mockup, drawn with what Home Assistant reports right now. A placeholder is the tile being
// dragged, drawn where it will land.
import { computed, nextTick } from "vue";
import { vDrag } from "../drag";
import { numberText, t, te } from "../i18n";
import { displayName, effectiveControls, isFull, isWide, pageOf, pageTarget, SLOTS_PER_PAGE } from "../model/layout";
import { clockText, glyph } from "../model/topbar";
import { clock24, entityName, isSelected, liveOf, numberMarks, openTile, placeTile, removeTile, screenBuiltinName, screenText, state, tileIconCp, unitSuffix } from "../store";
import type { Tile } from "../types";

const props = defineProps<{ tile: Tile; slot: number; placeholder?: boolean }>();
// A built-in card is named as the screens name it, in their language (app 0.2.90).
const name = computed(() => props.tile.name || (domain.value === "screen" && screenBuiltinName(props.tile.entity)) || entityName(props.tile.entity));
const full = computed(() => isFull(props.tile));
const wide = computed(() => isWide(props.tile) && !full.value);
const goesTo = computed(() => pageTarget(props.tile.entity));
const background = computed(() => state.inventory.backgrounds?.[props.tile.options?.background || ""]?.color);
const bare = computed(() => props.tile.options?.background === "none");
const display = computed(() => props.tile.options?.display || "standard");
const note = computed(() => (display.value !== "standard" ? displayName(display.value) : ""));
const controls = computed(() => effectiveControls(props.tile, state.inventory));
const domain = computed(() => props.tile.entity.split(".")[0]);
const cp = computed(() => state.inventory.icons?.controls || {});
const key = (n: string) => (cp.value[n] ? glyph(cp.value[n]) : "");
const chosen = computed(() => isSelected(props.tile) && state.inspector?.kind === "tile");
const live = computed(() => !props.placeholder && state.layout?.tiles.includes(props.tile));
const label = computed(() => t("editor.tile_card.label", { name: name.value, slot: (props.slot % SLOTS_PER_PAGE) + 1, page: pageOf(props.slot) + 1 }));
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
// Numbers as the screens write them, "1,234.5" or "1.234,5" (app 0.2.90): a state only with a unit, or of an entity
// that is a number itself, as the firmware does (value_text); an id-like "1234" without a unit stays as it is.
const num = (value: unknown) => numberText(value as string | number, numberMarks.value);
const NUMERIC = ["number", "input_number", "counter"];
const value = (state: string) => (unit.value || NUMERIC.includes(domain.value) ? `${num(state)}${unitSuffix(unit.value)}` : state);
// The screens' own words for a state where Home Assistant hands us none (screen.ha, Home Assistant's words in the
// screens' language, app 0.2.90): a binary sensor's by its device class, on and off, and the states of the domains
// the screen names itself. A weather's windy-variant is windy there too.
const HA_WORDS: Record<string, string> = { climate: "climate", cover: "cover", media_player: "media", person: "person", sun: "sun", vacuum: "vacuum", weather: "weather" };
function haWord(c: { state: string; a: Record<string, any> }) {
  const key = (path: string) => (te(`screen.ha.${path}`) ? screenText(`screen.ha.${path}`) : "");
  const value = c.state === "windy-variant" ? "windy" : c.state.replace(/-/g, "_");
  if (domain.value === "binary_sensor" && ["on", "off"].includes(value)) return key(`binary.${c.a?.device_class}_${value}`) || key(value);
  if (HA_WORDS[domain.value]) return key(`${HA_WORDS[domain.value]}.${value}`) || (["on", "off"].includes(value) ? key(value) : "");
  return ["on", "off"].includes(value) ? key(value) : "";
}
// A scene, script or button has no state worth a word: its state is the moment it last ran.
const NO_STATUS = ["scene", "script", "button", "input_button"];
// The text under the name: Home Assistant's word where it has one, the value with its unit for a sensor.
const status = computed(() => {
  const c = current.value;
  if (!c || NO_STATUS.includes(domain.value)) return note.value;
  if (gone.value) return screenText(c.state === "unknown" ? "editor.mockup.unknown" : "screen.ha.unavailable");
  const a = c.a || {};
  const word = c.word || haWord(c) || capital(c.state);
  if (domain.value === "climate") return `${a.current_temperature !== undefined ? `${num(a.current_temperature)}° · ` : ""}${word}`;
  if (domain.value === "weather") return `${word}${a.temperature !== undefined ? ` · ${num(a.temperature)}°` : ""}`;
  if (domain.value === "cover" && a.current_position !== undefined && a.current_position > 0 && a.current_position < 100) return `${word} · ${a.current_position}${unitSuffix("%")}`;
  if (domain.value === "media_player" && a.media_title) return `${word} · ${a.media_title}`;
  if (domain.value === "sensor" || NUMERIC.includes(domain.value)) return value(c.state);
  return word;
});
// The big value of the watch display; its unit sits beside it in small letters.
const bigValue = computed(() => (current.value && !gone.value ? (unit.value || NUMERIC.includes(domain.value) ? num(current.value.state) : current.value.state) : "—"));
// The small slider's fill, from what the entity reports; off is empty, like the screen's grey fill.
const fill = computed(() => {
  const c = current.value;
  if (!c || gone.value) return 0;
  const a = c.a || {};
  if (domain.value === "light") return c.state === "on" ? (a.brightness !== undefined ? Math.round((a.brightness / 255) * 100) : 100) : 0;
  if (domain.value === "fan") return c.state === "on" ? (a.percentage ?? 100) : 0;
  // A blind's bar fills with its closed part, as on the screen (firmware 0.2.66+) and in Home Assistant's cover dialog.
  if (domain.value === "cover") return 100 - (a.current_position ?? (c.state === "open" ? 100 : 0));
  if (domain.value === "media_player") return Math.round((a.volume_level ?? 0) * 100);
  if (domain.value === "number" || domain.value === "input_number") {
    const value = Number(c.state), min = Number(a.min ?? 0), max = Number(a.max ?? 100);
    return Number.isFinite(value) && max > min ? Math.round(((value - min) / (max - min)) * 100) : 0;
  }
  return 0;
});
// The key on a scene, script or button, and the page a navigation tile opens, as the screen labels them.
const runText = computed(() => screenText(`screen.ha.button.${({ scene: "activate", script: "run" } as Record<string, string>)[domain.value] || "press"}`));
const pageLink = computed(() => `${screenText("screen.tile.page", { n: goesTo.value })} ›`);
const sliderStyle = computed(() => ({ background: `linear-gradient(to right, ${fill.value ? "#ffbf38" : "#c9ccd1"} ${fill.value}%, ${fill.value ? "#fff1d3" : "#e6e8ec"} ${fill.value}%)` }));
const volumeStyle = computed(() => ({ background: `linear-gradient(to right, #2196f3 ${fill.value}%, #d3e8fb ${fill.value}%)` }));
const setpoint = computed(() => {
  const temperature = current.value?.a?.temperature;
  return temperature !== undefined && temperature !== null ? `${num(temperature)}°` : "—";
});

async function onKey(e: KeyboardEvent) {
  if (e.key === "Enter" || e.key === " ") { e.preventDefault(); openTile(props.tile); return; }
  const step = ({ ArrowLeft: -1, ArrowRight: 1, ArrowUp: -2, ArrowDown: 2 } as Record<string, number>)[e.key];
  if (!step) return;
  e.preventDefault();
  // A wide card owns its row: left and right mean the row above and below. A full card moves by the page.
  if (placeTile(props.tile, props.tile.slot + (full.value ? Math.sign(step) * SLOTS_PER_PAGE : wide.value ? Math.sign(step) * 2 : step))) {
    await nextTick();
    document.querySelector<HTMLElement>(`.pages [data-slot="${props.tile.slot}"]`)?.focus();
  }
}
</script>

<template>
  <div class="tile" :class="{ wide, full, bare, placeholder: placeholder || !live, chosen }" :data-slot="slot"
    :style="background && !bare ? { backgroundColor: background } : undefined"
    :tabindex="live ? 0 : -1" :role="live ? 'button' : undefined" :aria-label="live ? label : undefined"
    v-drag="{ kind: 'tile', tile }" @click="live && openTile(tile)" @keydown="live && onKey($event)">
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
      <span class="lead"><span class="big">{{ clockText(clock24, now) }}</span><span class="nm">{{ name }}</span></span>
    </template>
    <template v-else-if="full">
      <span class="ic mdi" :class="{ lit: isOn, thumb: display === 'live' }">{{ glyph(tileIconCp(tile)) }}</span>
      <span class="lead">
        <span class="nm">{{ name }}</span>
        <span v-if="goesTo" class="goto">{{ pageLink }}</span>
        <span v-else-if="display === 'watch'" class="big">{{ bigValue }}<small v-if="unit && !gone">{{ unit }}</small></span>
        <span v-else-if="status" class="st" :class="{ off: gone }">{{ status }}</span>
      </span>
      <span v-if="tile.options?.inline === 'slider'" class="mini-slider" :style="sliderStyle"></span>
      <span v-if="controls" class="ctl">
        <span v-if="controls === 'toggle'" class="tog" :class="{ off: !on }"></span>
        <span v-else-if="controls === 'setpoint'" class="stp"><span class="mdi">{{ key("minus") || "−" }}</span><b>{{ setpoint }}</b><span class="mdi">{{ key("plus") || "+" }}</span></span>
        <template v-else-if="controls === 'volume'"><span class="range" :style="volumeStyle"></span><span class="key mdi">{{ key("volume-high") }}</span></template>
        <template v-else-if="controls === 'playback'"><span class="key mdi">{{ key("skip-previous") }}</span><span class="key mdi">{{ key(on ? "pause" : "play") || key("play") }}</span><span class="key mdi">{{ key("skip-next") }}</span></template>
        <template v-else-if="controls === 'buttons' && domain === 'cover'"><span class="key mdi">{{ key("arrow-expand-horizontal") }}</span><span class="key mdi">{{ key("stop") }}</span><span class="key mdi">{{ key("arrow-collapse-horizontal") }}</span></template>
        <span v-else-if="controls === 'run'" class="run">{{ runText }}</span>
        <span v-else class="range" :style="{ background: `linear-gradient(to right, #2196f3 ${fill}%, #d3e8fb ${fill}%)` }"></span>
      </span>
    </template>
    <template v-else-if="wide">
      <span class="lead">
        <span class="ic mdi" :class="{ lit: isOn, thumb: display === 'live' }">{{ glyph(tileIconCp(tile)) }}</span>
        <span class="tx">
          <span class="nm">{{ name }}</span>
          <span v-if="goesTo" class="goto">{{ pageLink }}</span>
          <span v-else-if="display === 'watch'" class="big">{{ bigValue }}<small v-if="unit && !gone">{{ unit }}</small></span>
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
        <span v-else-if="controls === 'run'" class="run">{{ runText }}</span>
        <span v-else class="range" :style="{ background: `linear-gradient(to right, #2196f3 ${fill}%, #d3e8fb ${fill}%)` }"></span>
      </span>
    </template>
    <template v-else>
      <!-- As the screen draws it: the icon on the left, the name and the value beside it. A watch
           card puts the name on top and the big value under it; the small slider runs underneath. -->
      <span class="head" :class="{ top: display === 'watch' }">
        <span class="ic mdi" :class="{ lit: isOn, thumb: display === 'live' }">{{ glyph(tileIconCp(tile)) }}</span>
        <span class="tx">
          <span class="nm">{{ name }}</span>
          <span v-if="goesTo" class="goto">{{ pageLink }}</span>
          <span v-else-if="display !== 'watch' && status" class="st" :class="{ off: gone }">{{ status }}</span>
        </span>
      </span>
      <span v-if="display === 'watch'" class="big">{{ bigValue }}<small v-if="unit && !gone">{{ unit }}</small></span>
      <span v-if="tile.options?.inline === 'slider'" class="mini-slider" :style="sliderStyle"></span>
    </template>
    <button v-if="live" type="button" class="remove" :title="t('editor.tile_card.remove')" :aria-label="t('editor.tile_card.remove_named', { name })" @click.stop="removeTile(tile)">✕</button>
  </div>
</template>
