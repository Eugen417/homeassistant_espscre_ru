<script setup lang="ts">
// Alerts: the cheatsheet for esphome.<node>_show_alert, built from the inventory.
import { computed, reactive, ref } from "vue";
import { versionAtLeast } from "../model/layout";
import { glyph } from "../model/topbar";
import { canAlert, copyText, go, sendTestAlert, state } from "../store";

const alerts = computed(() => state.inventory.alerts);
// Try it: the same seven fields an automation sends, to one screen or to all of them.
const tryForm = reactive({ screen: "all", title: "Someone is at the door", subtitle: "Door 3, back", icon: "doorbell", color: "orange", button_text: "Coming", timeout: 30, flash: true });
const trying = ref(false);
const tryResult = ref("");
const readyScreens = computed(() => state.inventory.screens.filter((s) => canAlert(s) && s.online));
async function tryAlert() {
  trying.value = true;
  tryResult.value = "";
  try {
    const { screen, ...data } = tryForm;
    const result = await sendTestAlert(screen, data);
    const where = screen === "all" ? `${result.sent} screen${result.sent === 1 ? "" : "s"}` : state.inventory.screens.find((s) => s.id === screen)?.name || "the screen";
    tryResult.value = result.sent
      ? `Sent to ${where}.${result.skipped ? ` ${result.skipped} skipped (offline or old firmware).` : ""}${result.unusable?.length ? ` Left empty: ${result.unusable.join(", ")}.` : ""}`
      : "No screen could show it: check that a screen is online with firmware " + (alerts.value?.min_firmware || "0.2.31") + " or newer.";
  } catch (e: any) {
    tryResult.value = e.message;
  } finally {
    trying.value = false;
  }
}
const icons = computed(() => state.inventory.icons);
const exampleAction = ref("");
const iconQuery = ref("");
const yamlString = (text: unknown) => `"${String(text).replace(/\\/g, "\\\\").replace(/"/g, '\\"')}"`;
const fieldValue = (f: any) => f.type === "string" ? (/^[a-z][a-z0-9-]*$/.test(f.example) ? f.example : yamlString(f.example)) : f.example === true ? "true" : f.example === false ? "false" : String(f.example);
const screensWithAction = computed(() => state.inventory.screens.filter((s) => s.alert_action));
const chosenAction = computed(() => exampleAction.value || screensWithAction.value[0]?.alert_action || "");
const exampleYaml = computed(() => {
  const lines = (alerts.value?.fields || []).map((f: any) => `  ${f.name}: ${fieldValue(f)}`);
  return `action: ${chosenAction.value || "esphome.<device_name>_show_alert"}\ndata:\n${lines.join("\n")}`;
});
const waitYaml = computed(() => [
  `# Doorbell: show the alert and wait until someone presses the button.`,
  `actions:`,
  `  - action: ${chosenAction.value || "esphome.<device_name>_show_alert"}`,
  `    data:`,
  `      title: "Someone is at the door"`,
  `      subtitle: "Door 3, back"`,
  `      icon: doorbell`,
  `      color: orange`,
  `      button_text: "Coming"`,
  `      timeout: 0`,
  `      flash: true`,
  `  - wait_for_trigger:`,
  `      - trigger: event`,
  `        event_type: ${alerts.value?.event || "esphome.screen_alert"}`,
  `        event_data:`,
  `          action: ok`,
  `    timeout: "00:05:00"`,
  `  - if:`,
  `      - condition: template`,
  `        value_template: "{{ wait.trigger is not none }}"`,
  `    then:`,
  `      - action: notify.notify`,
  `        data:`,
  `          message: "Someone is coming to the door."`,
].join("\n"));
// One event for every screen (app 0.2.45): an action for "Edit in YAML" of the Event action.
const allYaml = computed(() => {
  const lines = (alerts.value?.fields || []).map((f: any) => `  ${f.name}: ${fieldValue(f)}`);
  const camera = alerts.value?.camera;
  if (camera) lines.push(`  # ${camera.name}: ${camera.example}   # a Guition shows its picture on the card`);
  return `event: ${alerts.value?.broadcast?.show || "esp_screens_show_alert"}\nevent_data:\n${lines.join("\n")}`;
});
const iconGroups = computed(() => {
  if (!alerts.value || !icons.value) return [];
  const query = iconQuery.value.trim().toLowerCase();
  const all = [...icons.value.groups, { label: "Also available: control and weather icons", icons: alerts.value.extra_icons }];
  return all.map((group) => ({ label: group.label, icons: group.icons.filter((i: any) => !query || i.name.includes(query) || (i.label || "").toLowerCase().includes(query)) })).filter((g) => g.icons.length);
});
const doorbell = computed(() => alerts.value?.suggested_icons?.find((i: any) => i.name === "doorbell"));
const orange = computed(() => alerts.value?.colors?.find((c: any) => c.name === "orange"));
const types: Record<string, string> = { string: "text", int: "number", bool: "on / off" };
function jump(id: string) { document.getElementById(id)?.scrollIntoView({ behavior: "smooth", block: "start" }); }
const sections = [["alerts-try", "Try it"], ["alerts-screens", "Your screens"], ["alerts-howto", "In an automation"], ["alerts-all", "All screens"], ["alerts-fields", "The fields"], ["alerts-icons", "Icons"], ["alerts-colors", "Colors"], ["alerts-behaviour", "Behavior"], ["alerts-events", "Events"], ["alerts-tips", "Tips"]];
</script>

<template>
  <div class="panel wide alerts" id="alerts-dialog">
    <div class="panel-head">
      <div class="tx">
        <span class="eyebrow">Alerts · cheatsheet</span>
        <h1>A full-screen notification, from an automation.</h1>
        <p>Every screen has the <code>show_alert</code> action. It places a card over everything, wakes the screen, and keeps the backlight at normal brightness until someone taps the button or the timeout runs out. Home Assistant itself shows no explanation or dropdowns for ESPHome actions; everything you can fill in is listed here.</p>
      </div>
      <button type="button" class="btn quiet" id="close-alerts" @click="go('')">← Back</button>
    </div>
    <p v-if="!alerts" class="hint">The cheatsheet is still loading; one moment.</p>
    <template v-else>
      <div class="alert-hero">
        <div class="alert-mock" id="alert-mock" aria-hidden="true">
          <div class="alert-mock-card" :style="orange ? { background: orange.color } : undefined">
            <span class="mdi alert-mock-icon" id="alert-mock-icon">{{ doorbell ? glyph(doorbell.cp) : "" }}</span>
            <strong id="alert-mock-title">Someone is at the door</strong>
            <span id="alert-mock-subtitle">Door 3, back</span>
            <span class="fake" id="alert-mock-button">Coming</span>
          </div>
        </div>
        <nav class="alerts-nav" aria-label="Sections">
          <button v-for="[id, label] in sections" :key="id" type="button" :data-jump="id" @click="jump(id)">{{ label }}</button>
        </nav>
      </div>
      <section id="alerts-try" class="card">
        <h2>Try it</h2>
        <p>Send one alert now, with the same fields an automation sends. The screen shows the card and, with blinking on, flashes its backlight.</p>
        <form class="try-grid" @submit.prevent="tryAlert">
          <div class="field"><label class="f-label" for="try-screen">Screen</label>
            <select id="try-screen" v-model="tryForm.screen">
              <option value="all">All screens ({{ readyScreens.length }} ready)</option>
              <option v-for="screen in state.inventory.screens" :key="screen.id" :value="screen.id" :disabled="!canAlert(screen) || !screen.online">{{ screen.name }}{{ canAlert(screen) && screen.online ? "" : " · not ready" }}</option>
            </select></div>
          <div class="field"><label class="f-label" for="try-title">Title</label><input id="try-title" v-model="tryForm.title" maxlength="64" /></div>
          <div class="field"><label class="f-label" for="try-subtitle">Subtitle</label><input id="try-subtitle" v-model="tryForm.subtitle" maxlength="240" /></div>
          <div class="field"><label class="f-label" for="try-icon">Icon</label><input id="try-icon" v-model="tryForm.icon" list="try-icons" placeholder="doorbell" />
            <datalist id="try-icons"><option v-for="icon in alerts.suggested_icons" :key="icon.name" :value="icon.name">{{ icon.label || icon.name }}</option></datalist></div>
          <div class="field"><label class="f-label" for="try-color">Color</label>
            <select id="try-color" v-model="tryForm.color"><option value="">White (default)</option><option v-for="colour in alerts.colors" :key="colour.name" :value="colour.name">{{ colour.label }}</option></select></div>
          <div class="field"><label class="f-label" for="try-button">Button text</label><input id="try-button" v-model="tryForm.button_text" maxlength="16" /></div>
          <div class="field"><label class="f-label" for="try-timeout">Timeout (seconds, 0 waits for the button)</label><input id="try-timeout" v-model.number="tryForm.timeout" type="number" min="0" max="86400" /></div>
          <label class="check field"><input type="checkbox" id="try-flash" v-model="tryForm.flash" /><span>Blink the backlight<small>Four times when the alert arrives.</small></span></label>
          <div class="actions field wide">
            <button type="submit" class="btn primary" id="try-send" :disabled="trying || !readyScreens.length">{{ trying ? "Sending…" : "Send alert" }}</button>
            <span class="status-line" id="try-result" role="status">{{ tryResult }}</span>
          </div>
        </form>
      </section>
      <section id="alerts-screens" class="card">
        <h2>Your screens</h2>
        <p>Every screen has its own action, with the device name in it. From firmware <code id="alerts-min-firmware">{{ alerts.min_firmware }}</code>; update an older screen with its Update button under Screens.</p>
        <div id="alerts-screen-list" class="options">
          <p v-if="!state.inventory.screens.length" class="hint">No screen paired yet. Install one via New screen; the action appears once Home Assistant sees the screen.</p>
          <div v-for="screen in state.inventory.screens" :key="screen.id" class="alert-screen">
            <div class="alert-screen-head">
              <strong>{{ screen.name }}</strong>
              <span class="chip" :class="versionAtLeast(screen.firmware, alerts.min_firmware) && screen.alert_action ? 'good' : 'update'">
                {{ versionAtLeast(screen.firmware, alerts.min_firmware) && screen.alert_action ? `● firmware ${screen.firmware}` : screen.alert_action ? `Update needed · firmware ${screen.firmware || "unknown"}` : "Device name unknown · update the screen" }}
              </span>
            </div>
            <div v-for="[label, action] in [['Show alert', screen.alert_action], ['Dismiss alert', screen.dismiss_action]]" :key="label" class="copy-line">
              <span class="copy-label">{{ label }}</span><code>{{ action || "esphome.<device_name>_show_alert" }}</code>
              <button v-if="action" type="button" class="btn quiet mini" @click="copyText(action!, undefined, 'Action name')">Copy</button>
            </div>
          </div>
        </div>
      </section>
      <section id="alerts-howto" class="card">
        <h2>In an automation</h2>
        <ol class="steps">
          <li><b>Add action</b> and search for <b>ESPHome</b>. Choose your screen's action: Home Assistant calls it "Performs the action show_alert of the node <i>device name</i>".</li>
          <li><b>Fill in the fields.</b> Home Assistant asks for all seven; leave what you don't use empty (<code>""</code>, <code>0</code>, off).</li>
          <li>Prefer YAML? Choose <b>Edit in YAML</b> on the action and paste the example below.</li>
        </ol>
        <div class="field">
          <label class="f-label" for="alerts-example-screen">Example for</label>
          <select id="alerts-example-screen" v-model="exampleAction" style="max-width: 360px">
            <option v-for="screen in screensWithAction" :key="screen.id" :value="screen.alert_action">{{ screen.name }}</option>
            <option v-if="!screensWithAction.length" value="">a screen (not paired yet)</option>
          </select>
        </div>
        <div class="copy-line">
          <pre id="alerts-example">{{ exampleYaml }}</pre>
          <button type="button" class="btn quiet mini" id="alerts-example-copy" @click="copyText(exampleYaml, undefined, 'YAML')">Copy YAML</button>
        </div>
      </section>
      <section id="alerts-all" class="card">
        <h2>All screens at once</h2>
        <p>Fire the event <code id="alerts-all-event">{{ alerts.broadcast?.show || "esp_screens_show_alert" }}</code> with the same fields, and ESP Screens passes the alert on to every screen that is online, including screens you add later. <code id="alerts-all-dismiss">{{ alerts.broadcast?.dismiss || "esp_screens_dismiss_alert" }}</code> clears it everywhere. In an automation, add the <b>Event</b> action or paste the example under <b>Edit in YAML</b>. Leave out the fields you don't use. ESP Screens has to be running for this.</p>
        <div class="copy-line">
          <pre id="alerts-all-example">{{ allYaml }}</pre>
          <button type="button" class="btn quiet mini" id="alerts-all-copy" @click="copyText(allYaml, undefined, 'YAML')">Copy YAML</button>
        </div>
      </section>
      <section id="alerts-fields" class="card">
        <h2>The fields</h2>
        <p>Text is free-form; the limit is in bytes, and an accented letter counts as two. What doesn't fit gets an ellipsis (title) or is dropped (subtitle).</p>
        <div class="table-scroll">
          <table id="alerts-field-table">
            <tr><th>Field</th><th>Type</th><th>What it does</th><th>Example</th><th>Limit</th></tr>
            <tr v-for="field in alerts.fields" :key="field.name">
              <td><code>{{ field.name }}</code><small>{{ field.label }}</small></td>
              <td>{{ types[field.type] || field.type }}</td>
              <td>{{ field.help }}</td>
              <td><code>{{ typeof field.example === "string" ? field.example : String(field.example) }}</code></td>
              <td>{{ alerts.limits.cyd[field.name] ? `CYD ${alerts.limits.cyd[field.name]} · Guition ${alerts.limits.guition[field.name]} bytes` : field.type === "int" ? "0 to 86400 s" : "—" }}</td>
            </tr>
            <tr v-if="alerts.camera">
              <td><code>{{ alerts.camera.name }}</code><small>{{ alerts.camera.label }}</small></td>
              <td>entity</td>
              <td>{{ alerts.camera.help }}</td>
              <td><code>{{ alerts.camera.example }}</code></td>
              <td>Guition, firmware 0.2.57+</td>
            </tr>
          </table>
        </div>
      </section>
      <section id="alerts-icons" class="card">
        <h2>Icons</h2>
        <p>Fill in the name in <code>icon</code>, for example <code>doorbell</code>. <code>mdi:doorbell</code> and the hex codepoint (<code>F12E6</code>) also work. Only these icons are included in the firmware; any other name shows the warning triangle. Tap an icon to copy its name.</p>
        <p class="subhead">Handy for notifications</p>
        <div class="chips" id="alerts-suggested">
          <button v-for="icon in alerts.suggested_icons" :key="icon.name" type="button" class="chip" @click="copyText(icon.name, undefined, 'Icon name')"><span class="mdi">{{ glyph(icon.cp) }}</span><code>{{ icon.name }}</code></button>
        </div>
        <div class="field">
          <label class="f-label" for="alerts-icon-search">Search for an icon</label>
          <input id="alerts-icon-search" v-model="iconQuery" type="search" placeholder="Search by name or description…" autocomplete="off" />
        </div>
        <div id="alerts-icon-groups">
          <div v-for="group in iconGroups" :key="group.label" class="field">
            <p class="subhead">{{ group.label }} · {{ group.icons.length }}</p>
            <div class="alert-icon-grid">
              <button v-for="icon in group.icons" :key="icon.name" type="button" class="alert-icon" :title="`Copy ${icon.name}`" @click="copyText(icon.name, undefined, 'Icon name')">
                <span class="mdi">{{ glyph(icon.cp) }}</span><code>{{ icon.name }}</code><small v-if="icon.label">{{ icon.label }}</small>
              </button>
            </div>
          </div>
          <p v-if="!iconGroups.length" class="hint">No icon for "{{ iconQuery }}". Unknown names show the warning triangle ({{ alerts.fallback_icon }}).</p>
        </div>
      </section>
      <section id="alerts-colors" class="card">
        <h2>Colors</h2>
        <p>Fill in the name in <code>color</code>. These are the pastel shades of the tiles, with dark text. Empty or unknown gives the white card. Tap to copy.</p>
        <div class="swatches" id="alerts-swatches">
          <button type="button" class="swatch" @click="copyText('', undefined, 'Empty color')"><i style="background: #ffffff"></i><span><code>empty</code><small>White (default)</small></span></button>
          <button v-for="colour in alerts.colors" :key="colour.name" type="button" class="swatch" @click="copyText(colour.name, undefined, 'Color name')"><i :style="{ background: colour.color }"></i><span><code>{{ colour.name }}</code><small>{{ colour.label }} · {{ colour.color }}</small></span></button>
        </div>
      </section>
      <section id="alerts-behaviour" class="card">
        <h2>Behavior</h2>
        <ul class="alerts-list">
          <li><b>Wakes the screen.</b> An alert immediately wakes a dimmed or dark screen and appears above every page, every open light card, and the standby layer.</li>
          <li><b>On until the button.</b> As long as the card is showing, the backlight stays at normal brightness. Standby and night hours wait. After the button, the regular standby time starts again.</li>
          <li><b>Timeout.</b> <code>timeout: 0</code> waits for the button, however long that takes. With <code>timeout: 60</code> the card disappears after a minute, but the button closes it sooner.</li>
          <li><b>Blinking.</b> <code>flash: true</code> blinks the backlight four times when the alert arrives (about a second) and leaves the screen on afterward.</li>
          <li><b>Replacing.</b> A new alert replaces the current one; there's no stack. The replaced alert reports itself as <code>replaced</code>.</li>
          <li><b>Closing remotely.</b> <code>dismiss_alert</code> clears the card, for example if the door is already open. Without an alert, that action does nothing.</li>
          <li><b>Tapping beside the card</b> does nothing; only the button closes it. That way a wake-up tap never accidentally dismisses the alert.</li>
        </ul>
      </section>
      <section id="alerts-events" class="card">
        <h2>Events</h2>
        <p>Every end fires the event <code id="alerts-event-name">{{ alerts.event }}</code> with <code>action</code>, <code>title</code>, <code>screen</code> (the device name) and the <code>device_id</code> that Home Assistant adds. Visible under Settings → Tools → Events.</p>
        <div class="table-scroll">
          <table id="alerts-ending-table">
            <tr><th>action</th><th>When</th></tr>
            <tr v-for="ending in alerts.endings" :key="ending.action"><td><code>{{ ending.action }}</code></td><td>{{ ending.label }}</td></tr>
          </table>
        </div>
        <p class="subhead">Waiting until someone presses the button</p>
        <div class="copy-line">
          <pre id="alerts-wait-example">{{ waitYaml }}</pre>
          <button type="button" class="btn quiet mini" id="alerts-wait-copy" @click="copyText(waitYaml, undefined, 'YAML')">Copy YAML</button>
        </div>
      </section>
      <section id="alerts-tips" class="card">
        <h2>Tips</h2>
        <ul class="alerts-list">
          <li>Use a <b>color per kind of notification</b>: orange for the door, red for smoke or water, green for "done". The text always stays dark and readable.</li>
          <li>Keep the title <b>short</b>; a screen shows about twenty characters on one line and ends a longer title with an ellipsis. Put details in the subtitle.</li>
          <li>A <b>custom button text</b> turns the notification into a question: "Coming", "Seen", "Turn off". The event then tells you who responded.</li>
          <li>For an alert that resolves itself (the door opens, the laundry gets taken out) close it in the same automation with <code>dismiss_alert</code>.</li>
          <li>Every screen? Fire <code>{{ alerts.broadcast?.show || "esp_screens_show_alert" }}</code> once (see All screens at once). The screens still work independently and each reports with its own <code>screen</code>.</li>
          <li>Rather <b>ask Claude</b> for the automation? Install the ESP Screens skill under Settings → Claude.</li>
        </ul>
      </section>
    </template>
  </div>
</template>
