<script setup lang="ts">
defineProps<{ choices: readonly (readonly [unknown, string])[]; value: unknown; disabled?: boolean }>();
const emit = defineEmits<{ (e: "pick", value: any): void }>();
</script>

<template>
  <div class="seg wrap" role="group">
    <button v-for="[key, text] in choices" :key="String(key)" type="button" :disabled="disabled"
      :aria-pressed="String(key) === String(value) ? 'true' : 'false'" @click="emit('pick', key)">{{ text }}</button>
  </div>
</template>
