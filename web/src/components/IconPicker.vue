<script setup lang="ts">
// The icon choice for tiles and top bar items: automatic, optionally none, or one from the set.
// A choice applies live; the open picker keeps its search and scroll position.
import { computed, ref } from "vue";
import { glyph } from "../model/topbar";
import { iconNamed, state } from "../store";

const props = defineProps<{ selected: string; automatic: string; autoLabel: string; allowNone?: boolean; note?: string }>();
const emit = defineEmits<{ (e: "pick", name: string): void }>();
const query = ref("");
const chosen = computed(() => iconNamed(props.selected));
const currentGlyph = computed(() => (props.selected === "none" ? "" : glyph(chosen.value?.cp || props.automatic)));
const currentText = computed(() => (props.selected === "none" ? "No icon" : chosen.value?.label || props.autoLabel));
const groups = computed(() => {
  const q = query.value.trim().toLocaleLowerCase();
  return (state.inventory.icons?.groups || []).map((group) => ({
    label: group.label,
    icons: group.icons.filter((i) => !q || `${i.label} ${i.name.replaceAll("-", " ")} ${group.label}`.toLocaleLowerCase().includes(q)),
  })).filter((g) => g.icons.length);
});
</script>

<template>
  <div class="f">
    <span class="f-label">Icon</span>
    <button type="button" class="row" :aria-expanded="state.iconPickerOpen ? 'true' : 'false'" @click="state.iconPickerOpen = !state.iconPickerOpen">
      <span class="av mdi">{{ currentGlyph }}</span>
      <span class="tx"><b>{{ currentText }}</b></span>
      <span class="link">{{ state.iconPickerOpen ? "Close" : "Change" }}</span>
    </button>
    <div v-if="state.iconPickerOpen" class="picker">
      <input v-model="query" type="search" placeholder="Search, for example lamp, music, or door" aria-label="Search for an icon" />
      <button type="button" class="icon-choice icon-auto" :aria-pressed="selected === 'auto' ? 'true' : 'false'" :title="autoLabel" @click="emit('pick', 'auto')">
        <span class="mdi">{{ glyph(automatic) }}</span><span>{{ autoLabel }}</span>
      </button>
      <button v-if="allowNone" type="button" class="icon-choice icon-auto" :aria-pressed="selected === 'none' ? 'true' : 'false'" title="No icon" @click="emit('pick', 'none')">
        <span class="mdi"></span><span>No icon, text only</span>
      </button>
      <div class="icon-list">
        <section v-for="group in groups" :key="group.label">
          <small>{{ group.label }}</small>
          <div class="icon-grid">
            <button v-for="icon in group.icons" :key="icon.name" type="button" class="icon-choice" :title="icon.label" :aria-label="`Icon: ${icon.label}`"
              :aria-pressed="selected === icon.name ? 'true' : 'false'" @click="emit('pick', icon.name)"><span class="mdi">{{ glyph(icon.cp) }}</span></button>
          </div>
        </section>
        <p v-if="!groups.length" class="hint">No icon found.</p>
      </div>
    </div>
    <small v-if="note">{{ note }}</small>
  </div>
</template>
