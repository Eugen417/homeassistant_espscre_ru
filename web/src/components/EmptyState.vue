<script setup lang="ts">
import { go, state } from "../store";
</script>

<template>
  <section v-if="state.inventory.screens.length" id="choose" class="empty">
    <span class="empty-icon">▦</span>
    <h2>Choose a screen</h2>
    <p>Pick one of your screens on the left to change its tiles, top bar and settings.</p>
  </section>
  <section v-else id="empty" class="empty">
    <span class="empty-icon">▦</span>
    <h2>Start with a screen</h2>
    <p>Connect a CYD or Guition via USB to your Home Assistant machine. Profile, firmware, and the first flash happen in one go.</p>
    <button id="start" type="button" class="btn primary" @click="go('#new-screen')">Install new screen</button>
  </section>
</template>
