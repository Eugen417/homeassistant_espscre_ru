<script setup lang="ts">
import { dismissToast, state } from "../store";
</script>

<template>
  <div v-if="state.toast" id="toast" class="toast" role="status">
    <span>{{ state.toast.message }}</span>
    <button v-if="state.toast.action" type="button" @click="state.toast.action.run(); dismissToast()">{{ state.toast.action.label }}</button>
  </div>
</template>
