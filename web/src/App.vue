<script setup lang="ts">
import { computed } from "vue";
import Sidebar from "./components/Sidebar.vue";
import Toast from "./components/Toast.vue";
import ScreenView from "./components/ScreenView.vue";
import EmptyState from "./components/EmptyState.vue";
import AppSettingsView from "./components/AppSettingsView.vue";
import InstallerView from "./components/InstallerView.vue";
import FirmwareView from "./components/FirmwareView.vue";
import AlertsView from "./components/AlertsView.vue";
import OverrideView from "./components/OverrideView.vue";
import { currentScreen, route, state } from "./store";

const view = computed(() => {
  if (route.value === "#settings") return AppSettingsView;
  if (route.value === "#new-screen") return InstallerView;
  if (route.value === "#firmware") return FirmwareView;
  if (route.value === "#alerts") return AlertsView;
  if (route.value === "#override") return OverrideView;
  return currentScreen.value && state.layout ? ScreenView : EmptyState;
});
</script>

<template>
  <div class="app" :class="{ dragging: state.drag.active }">
    <Sidebar />
    <main class="main">
      <component :is="view" />
    </main>
    <Toast />
  </div>
</template>
