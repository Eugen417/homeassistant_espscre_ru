<script setup lang="ts">
// A card on the mockup. A placeholder is the tile being dragged, drawn where it will land.
import { computed, nextTick } from "vue";
import { vDrag } from "../drag";
import { displayNames, effectiveControls, isWide, pageOf, SLOTS_PER_PAGE } from "../model/layout";
import { glyph } from "../model/topbar";
import { entityName, openTile, placeTile, removeTile, state, tileIconCp } from "../store";
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
    <template v-else-if="wide">
      <span class="lead">
        <span class="ic mdi">{{ glyph(tileIconCp(tile)) }}</span>
        <span class="tx"><span class="nm">{{ name }}</span><span v-if="note" class="st">{{ note }}</span></span>
      </span>
      <span v-if="tile.options?.inline === 'slider'" class="mini-slider"></span>
      <span v-if="controls" class="ctl">
        <span v-if="controls === 'toggle'" class="tog"></span>
        <span v-else-if="controls === 'setpoint'" class="stp"><span class="mdi">{{ key("minus") || "−" }}</span><b>20°</b><span class="mdi">{{ key("plus") || "+" }}</span></span>
        <template v-else-if="controls === 'stepper' && domain.endsWith('select')"><span class="key mdi">{{ key("chevron-left") }}</span><span class="key mdi">{{ key("chevron-right") }}</span></template>
        <span v-else-if="controls === 'stepper'" class="stp"><span class="mdi">{{ key("minus") || "−" }}</span><b>50</b><span class="mdi">{{ key("plus") || "+" }}</span></span>
        <template v-else-if="controls === 'mode'"><span class="key mdi">{{ key("power") }}</span><span class="key mdi">{{ key("fire") }}</span><span class="key mdi">{{ key("snowflake") }}</span></template>
        <template v-else-if="controls === 'volume'"><span class="range"></span><span class="key mdi">{{ key("volume-high") }}</span></template>
        <template v-else-if="controls === 'playback'"><span class="key mdi">{{ key("skip-previous") }}</span><span class="key mdi">{{ key("play") }}</span><span class="key mdi">{{ key("skip-next") }}</span></template>
        <template v-else-if="controls === 'buttons' && domain === 'cover'"><span class="key mdi">{{ key("arrow-expand-horizontal") }}</span><span class="key mdi">{{ key("stop") }}</span><span class="key mdi">{{ key("arrow-collapse-horizontal") }}</span></template>
        <template v-else-if="controls === 'buttons' && domain === 'vacuum'"><span class="key mdi">{{ key("play") }}</span><span class="key mdi">{{ key("stop") }}</span><span class="key mdi">{{ key("home-map-marker") }}</span></template>
        <template v-else-if="controls === 'buttons' && domain === 'timer'"><span class="key mdi">{{ key("play") }}</span><span class="key mdi">{{ key("close") }}</span></template>
        <span v-else-if="controls === 'run'" class="run">{{ ({ scene: "Activate", script: "Run" } as Record<string, string>)[domain] || "Press" }}</span>
        <span v-else class="range"></span>
      </span>
    </template>
    <template v-else>
      <span class="ic mdi">{{ glyph(tileIconCp(tile)) }}</span>
      <span class="lead">
        <span class="nm">{{ name }}</span>
        <span v-if="note" class="st">{{ note }}</span>
        <span v-if="tile.options?.inline === 'slider'" class="mini-slider"></span>
      </span>
    </template>
    <button v-if="live" type="button" class="remove" title="Remove tile" :aria-label="`Remove ${name}`" @click.stop="removeTile(tile)">✕</button>
  </div>
</template>
