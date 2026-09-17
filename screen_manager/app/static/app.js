"use strict";
const $ = (s) => document.querySelector(s);
let inventory = { screens: [], entities: [] },
  selected = null,
  layout = null,
  dirty = false,
  filter = "",
  busy = false,
  selectedTile = null;
const node = (tag, text, cls) => {
  const n = document.createElement(tag);
  if (text !== undefined) n.textContent = text;
  if (cls) n.className = cls;
  return n;
};
function toast(message, action) {
  const t = $("#toast");
  t.replaceChildren(node("span", message));
  if (action) {
    const b = node("button", action.label);
    b.type = "button";
    b.onclick = () => { action.run(); t.hidden = true; };
    t.append(b);
  }
  t.hidden = false;
  clearTimeout(toast.timer);
  toast.timer = setTimeout(() => (t.hidden = true), action ? 8000 : 5000);
}
async function api(path, options = {}) {
  const response = await fetch(`api/${path}`, {
    ...options,
    headers: {
      "Content-Type": "application/json",
      "X-Screen-CSRF": inventory.csrf || "",
      ...options.headers,
    },
  });
  if (!response.ok) {
    let message = "That didn't work. Refresh the page and try again.";
    try {
      message = (await response.json()).error || message;
    } catch {}
    throw new Error(message);
  }
  return response;
}
function markDirty() {
  dirty = true;
  $("#dirty").textContent = "Unsaved changes";
  $("#save-detail").textContent = "Click save to update your screen.";
}
function select(id) {
  if (
    id !== selected &&
    dirty &&
    !confirm(
      "You have unsaved changes. Open a different screen anyway?",
    )
  )
    return;
  if (id !== selected) {
    flushSettings();
    settingEdits = {};
  }
  selected = id;
  selectedTile = null;
  if ($("#tile-sheet").open) $("#tile-sheet").close();
  const screen = inventory.screens.find((s) => s.id === id);
  if (!screen) return;
  layout = structuredClone(screen.layout);
  normalize();
  insertAt = -1;
  dirty = false;
  $("#title").value = layout.title;
  $("#screen-name").textContent = screen.name;
  $("#open-override").disabled = !screen.update?.profile;
  $("#editor").hidden = false;
  $("#empty").hidden = true;
  $("#dirty").textContent = "All saved";
  $("#save-detail").textContent = "Changes apply without reflashing.";
  renderScreens();
  renderTopbar();
  renderTiles();
  renderResults();
  renderSettings();
  loadTopbarPreview(0);
}
// ----- Screen settings: the same groups and rows as the settings page on the screen itself -----
// Every change applies at once, like on the screen; no Save needed. A screen with firmware 0.2.49+ owns its
// settings and ESP Screens changes them through its entities in Home Assistant, so an automation, the page on
// the screen and this panel always show the same value. Older firmware gets them with its layout.
const SETTING_GROUPS = [
  { title: "Brightness", icon: "F0599", rows: [
    { key: "brightness", label: "Brightness", kind: "number", min: 5, max: 100, step: 5, unit: "%" },
    { key: "dark_mode", label: "Dark mode", kind: "toggle" },
    { key: "standby_enabled", label: "Auto standby", kind: "toggle" },
    { key: "standby_seconds", label: "Standby after", kind: "duration", min: 60, max: 86400, needs: "standby_enabled" },
    { key: "standby_brightness", label: "Standby brightness", kind: "number", min: 0, max: 100, step: 5, unit: "%", needs: "standby_enabled", cap: "brightness" },
  ] },
  { title: "Night", icon: "F0594", rows: [
    { key: "night_enabled", label: "Night mode", kind: "toggle" },
    { key: "night_start", label: "Starts", kind: "moment", needs: "night_enabled" },
    { key: "night_end", label: "Ends", kind: "moment", needs: "night_enabled" },
    { key: "night_brightness", label: "Night brightness", kind: "number", min: 0, max: 100, step: 5, unit: "%", needs: "night_enabled", cap: "brightness" },
  ] },
  { title: "Screen", icon: "F0379", rows: [
    { key: "clock_24h", label: "Clock", kind: "choice", options: [[false, "12 hour"], [true, "24 hour"]] },
    { key: "auto_home", label: "Back to page 1", kind: "toggle" },
    { key: "auto_home_seconds", label: "After", kind: "duration", min: 30, max: 3600, needs: "auto_home" },
    { key: "home_on_standby", label: "Also on standby", kind: "toggle" },
    { key: "swipe_pages", label: "Swipe between pages", kind: "toggle" },
    { key: "rotation", label: "Rotation", kind: "choice", options: [[0, "0°"], [90, "90°"], [180, "180°"], [270, "270°"]] },
  ] },
];
const SETTING_ROWS = Object.fromEntries(SETTING_GROUPS.flatMap((group) => group.rows.map((row) => [row.key, row])));
// Changes made here that the screen has not reported back yet: {key: {value, at}}. They win over what
// Home Assistant still shows for a few seconds, so a value never flicks back while it travels.
let settingEdits = {}, settingQueue = {}, settingTarget = null, settingTimer = null, settingFlight = null, settingPanel = { key: "", rows: {} };
const SETTING_EDIT_MS = 4000;
const settingsView = () => inventory.screens.find((s) => s.id === selected)?.settings;
function settingValues() {
  const view = settingsView(), values = { ...(view?.values || {}) };
  for (const [key, edit] of Object.entries(settingEdits)) values[key] = edit.value;
  return values;
}
// The same steps as settings_screen.h: seconds low down, quarters of an hour up top; times by the quarter,
// whole hours while held.
const ladderStep = (seconds) => (seconds < 300 ? 30 : seconds < 900 ? 60 : seconds < 3600 ? 300 : seconds < 7200 ? 900 : 1800);
function steppedSetting(row, value, direction, held, values) {
  if (row.kind === "moment") {
    let next = held && value % 60 ? Math.floor(value / 60) * 60 + (direction > 0 ? 60 : 0) : value + direction * (held ? 60 : 15);
    next %= 1440;
    return next < 0 ? next + 1440 : next;
  }
  const step = row.kind === "duration" ? ladderStep(direction < 0 ? value - 1 : value) : row.step;
  const max = row.cap ? Math.min(row.max, values[row.cap]) : row.max;
  return Math.min(max, Math.max(row.min, value + direction * step));
}
function durationText(seconds) {
  if (seconds < 60) return `${seconds} sec`;
  if (seconds < 3600) return `${Math.floor(seconds / 60)} min`;
  const hours = Math.floor(seconds / 3600), minutes = Math.floor((seconds % 3600) / 60);
  return minutes ? `${hours} h ${String(minutes).padStart(2, "0")}` : `${hours} h`;
}
function momentText(minutes, clock24) {
  const hour = Math.floor(minutes / 60), minute = String(minutes % 60).padStart(2, "0");
  return clock24 ? `${String(hour).padStart(2, "0")}:${minute}` : `${hour % 12 || 12}:${minute} ${hour < 12 ? "AM" : "PM"}`;
}
function settingText(row, values) {
  const value = values[row.key];
  // Home Assistant has no value while the screen is offline or the entity is off.
  if (value === null || value === undefined) return "—";
  if (row.kind === "number") return `${value}${row.unit || ""}`;
  if (row.kind === "duration") return durationText(value);
  if (row.kind === "moment") return momentText(value, values.clock_24h !== false);
  return "";
}
// A key held down steps again and again, faster after a moment, like the -/+ keys on the screen.
function holdable(button, step) {
  let timer = null, repeats = 0, held = false;
  const stop = () => { clearTimeout(timer); timer = null; };
  button.addEventListener("pointerdown", (event) => {
    if (button.disabled || event.button !== 0) return;
    held = false;
    repeats = 0;
    stop();
    timer = setTimeout(function repeat() {
      held = true;
      repeats += 1;
      step(repeats > 5);
      timer = setTimeout(repeat, 180);
    }, 450);
  });
  for (const type of ["pointerup", "pointercancel", "pointerleave"]) button.addEventListener(type, stop);
  button.addEventListener("click", () => {
    if (held) { held = false; return; }
    step(false);
  });
}
function settingControl(row) {
  const refs = { row };
  const line = node("div", undefined, `setting-row setting-${row.kind}`);
  line.dataset.setting = row.key;
  const label = node("span", row.label, "setting-label");
  label.id = `setting-label-${row.key}`;
  const control = node("div", undefined, "setting-control");
  if (row.kind === "toggle") {
    const toggle = node("button", undefined, "switch");
    toggle.type = "button";
    toggle.id = `setting-${row.key}`;
    toggle.setAttribute("role", "switch");
    toggle.setAttribute("aria-labelledby", label.id);
    toggle.onclick = () => setSetting(row.key, !settingValues()[row.key], 150);
    // The whole row answers a click, as on the screen.
    line.onclick = (event) => { if (event.target === line || event.target === label) toggle.click(); };
    control.append(toggle);
    refs.toggle = toggle;
  } else if (row.kind === "choice") {
    const group = node("div", undefined, "segmented setting-choices");
    group.setAttribute("role", "group");
    group.setAttribute("aria-labelledby", label.id);
    refs.chips = row.options.map(([value, text]) => {
      const chip = node("button", text);
      chip.type = "button";
      chip.onclick = () => setSetting(row.key, value, 150);
      group.append(chip);
      return [value, chip];
    });
    control.append(group);
  } else {
    const minus = node("button", undefined, "step");
    const plus = node("button", undefined, "step");
    minus.type = plus.type = "button";
    minus.append(node("span", glyph("F0374"), "mdi"));
    plus.append(node("span", glyph("F0415"), "mdi"));
    minus.setAttribute("aria-label", `${row.label} lower`);
    plus.setAttribute("aria-label", `${row.label} higher`);
    const value = node("output", undefined, "setting-value");
    value.setAttribute("aria-labelledby", label.id);
    value.id = `setting-${row.key}`;
    for (const [button, direction] of [[minus, -1], [plus, 1]]) {
      holdable(button, (held) => {
        const values = settingValues();
        const next = steppedSetting(row, values[row.key], direction, held, values);
        if (next !== values[row.key]) setSetting(row.key, next, 600);
      });
    }
    control.append(minus, value, plus);
    Object.assign(refs, { minus, plus, value });
  }
  line.append(label, control);
  refs.line = line;
  return refs;
}
function renderSettings() {
  const container = $("#settings-groups");
  const screen = inventory.screens.find((s) => s.id === selected), view = screen?.settings;
  if (!view) {
    container.replaceChildren();
    settingPanel = { key: "", rows: {} };
    $("#settings-status").textContent = "";
    return;
  }
  // Rebuild only when the rows change (another screen, board or firmware); otherwise update in place, so a
  // key held down keeps its grip while the values come back.
  const shape = `${screen.id}|${view.keys.join(",")}`;
  if (settingPanel.key !== shape) {
    const rows = {};
    container.replaceChildren(...SETTING_GROUPS.map((group) => {
      const shown = group.rows.filter((row) => view.keys.includes(row.key));
      const card = node("section", undefined, "settings-card");
      card.hidden = !shown.length;
      const head = node("h4");
      head.append(node("span", glyph(group.icon), "mdi"), node("span", group.title));
      card.append(head);
      for (const row of shown) {
        rows[row.key] = settingControl(row);
        card.append(rows[row.key].line);
      }
      return card;
    }));
    settingPanel = { key: shape, rows };
  }
  const values = settingValues();
  const offline = view.owner === "screen" && !screen.online;
  for (const [key, refs] of Object.entries(settingPanel.rows)) {
    const row = refs.row, needs = row.needs ? values[row.needs] : true;
    const unavailable = offline || view.unavailable.includes(key);
    refs.line.classList.toggle("inactive", !needs || unavailable);
    refs.line.title = unavailable && !offline ? "This entity is off in Home Assistant, or the screen is restarting." : "";
    if (refs.toggle) {
      refs.toggle.setAttribute("aria-checked", String(Boolean(values[key])));
      refs.toggle.classList.toggle("unknown", values[key] === null || values[key] === undefined);
      refs.toggle.disabled = unavailable;
    } else if (refs.chips) {
      for (const [value, chip] of refs.chips) {
        chip.setAttribute("aria-pressed", String(values[key] === value));
        chip.disabled = unavailable;
      }
    } else {
      refs.value.textContent = settingText(row, values);
      const lower = steppedSetting(row, values[key], -1, false, values), higher = steppedSetting(row, values[key], 1, false, values);
      refs.minus.disabled = unavailable || !needs || (row.kind !== "moment" && lower === values[key]);
      refs.plus.disabled = unavailable || !needs || (row.kind !== "moment" && higher === values[key]);
    }
  }
  const pending = Object.keys(settingQueue).length || settingFlight;
  $("#settings-status").textContent = offline
    ? "This screen is offline. Its settings can change once it's back."
    : pending
      ? "Saving…"
      : view.owner === "screen"
        ? "Changes apply on the screen at once, and show up here when they change there."
        : "Changes apply at once. Firmware 0.2.49 lets the screen keep them itself.";
  $("#general-settings").classList.toggle("offline", offline);
}
function setSetting(key, value, delay) {
  // One screen's changes at a time: the ones for the screen shown before go out first.
  if (settingTarget && settingTarget !== selected && Object.keys(settingQueue).length) {
    flushSettings();
    toast("Still saving the other screen's settings. Try again in a moment.");
    return;
  }
  settingTarget = selected;
  const values = settingValues();
  settingEdits[key] = { value, at: Date.now() };
  settingQueue[key] = value;
  // A lower brightness pulls both dim levels down with it, as on the screen.
  if (key === "brightness")
    for (const dim of ["standby_brightness", "night_brightness"])
      if (values[dim] > value) settingEdits[dim] = { value, at: Date.now() };
  renderSettings();
  if (key === "clock_24h") { renderTopbar(); renderBars(); if ($("#topbar-sheet").open) renderTopbarLive(); }
  clearTimeout(settingTimer);
  settingTimer = setTimeout(flushSettings, delay);
}
async function flushSettings(unloading = false) {
  clearTimeout(settingTimer);
  if (settingFlight || !Object.keys(settingQueue).length || !settingTarget) return;
  const screen = settingTarget, changes = settingQueue;
  settingQueue = {};
  const request = api(`screens/${encodeURIComponent(screen)}/settings`, {
    method: "PUT",
    body: JSON.stringify({ settings: changes }),
    keepalive: unloading,
  });
  settingFlight = request;
  renderSettings();
  try {
    const view = await (await request).json();
    const current = inventory.screens.find((s) => s.id === screen);
    if (current) current.settings = view;
  } catch (e) {
    toast(e.message);
    // What did not arrive is not kept: the panel shows the screen's own values again.
    if (screen === selected) for (const key of Object.keys(changes)) delete settingEdits[key];
    if (screen === selected && changes.brightness !== undefined) for (const dim of ["standby_brightness", "night_brightness"]) delete settingEdits[dim];
  } finally {
    settingFlight = null;
    if (Object.keys(settingQueue).length) settingTimer = setTimeout(flushSettings, 150);
    else settingTarget = null;
    if (screen === selected) { settleSettings(); renderSettings(); }
    // A value the screen refused or clamped comes back without a live update: look again once edits expire.
    setTimeout(() => { if (screen === selected) { settleSettings(); renderSettings(); } }, SETTING_EDIT_MS + 100);
  }
}
// Values Home Assistant reports take over again once they match a change made here, or after a few seconds
// (the screen refused or clamped it).
function settleSettings() {
  const view = settingsView();
  for (const [key, edit] of Object.entries(settingEdits)) {
    if (settingQueue[key] !== undefined || settingFlight) continue;
    if ((view && view.values[key] === edit.value) || Date.now() - edit.at > SETTING_EDIT_MS) delete settingEdits[key];
  }
}
const updating = new Set();
const PHASES = {
  install: "Building and installing…",
  verify: "Waiting for the screen to come back…",
  settle: "Checking that it stays stable…",
};
async function startUpdate(screen, host) {
  updating.add(screen.id);
  renderScreens();
  try {
    await api(`screens/${encodeURIComponent(screen.id)}/update`, {
      method: "POST",
      body: JSON.stringify(host ? { host } : {}),
    });
    await refresh();
  } catch (e) {
    updating.delete(screen.id);
    renderScreens();
    toast(e.message);
  }
}
function renderUpdate(screen) {
  const u = screen.update || {};
  const row = node("div", undefined, "screen-update");
  const running = u.state === "running" || updating.has(screen.id);
  if (running) {
    row.append(
      node("span", undefined, "spin"),
      node("small", PHASES[u.phase] || "Starting update…"),
    );
  } else if (u.state === "queued") {
    row.append(node("small", "Queued for the update"));
  } else if (u.available && screen.online) {
    row.append(node("span", `Update ${u.target}`, "badge update"));
    const button = node("button", "Update", "mini");
    button.type = "button";
    if (u.host && u.profile) {
      button.onclick = (e) => {
        e.stopPropagation();
        startUpdate(screen);
      };
    } else if (!u.profile) {
      button.disabled = true;
      button.title = "No ESPHome profile found with this device name.";
    } else {
      button.onclick = (e) => {
        e.stopPropagation();
        const form = node("form", undefined, "screen-host");
        const input = node("input");
        input.placeholder = "IP address, e.g. 192.168.1.50";
        input.required = true;
        input.pattern = "[A-Za-z0-9][A-Za-z0-9.\\-]*";
        const go = node("button", "Start", "mini");
        const cancel = node("button", "✕", "quiet");
        cancel.type = "button";
        cancel.setAttribute("aria-label", "Cancel");
        cancel.onclick = () => {
          form.remove();
          renderScreens();
        };
        form.append(input, go, cancel);
        form.onsubmit = (ev) => {
          ev.preventDefault();
          form.remove();
          startUpdate(screen, input.value.trim());
        };
        row.replaceChildren(
          node("small", "The IP address, once; new firmware reports it itself."),
          form,
        );
        input.focus();
      };
    }
    row.append(button);
  } else if (u.result && Date.now() / 1000 - u.result.time < 86400) {
    row.append(
      node(
        "small",
        u.result.message,
        u.result.state === "failed" ? "failed" : "",
      ),
    );
  } else return null;
  return row;
}
function openIntegrations() {
  // Pairing happens in Home Assistant itself. This page lives in HA's ingress iframe,
  // so send the top window to Devices & services (same origin); elsewhere open a tab.
  const path = "/config/integrations/dashboard";
  try {
    window.top.location.assign(path);
  } catch {
    window.open(path, "_blank");
  }
}
async function copyText(text, element, what = "API key") {
  try {
    if (!navigator.clipboard || !window.isSecureContext) throw new Error();
    await navigator.clipboard.writeText(text);
    toast(`${what} copied.`);
  } catch {
    if (element) {
      const range = document.createRange();
      range.selectNodeContents(element);
      const selection = window.getSelection();
      selection.removeAllRanges();
      selection.addRange(range);
    }
    toast(document.execCommand("copy") ? `${what} copied.` : `${what} is selected. Copy with Ctrl+C or Command+C.`);
  }
}
// Profiles that Home Assistant does not list yet: the freshly flashed screen is not lost, it
// still has to be added under Devices & services, outside this page.
function renderPending() {
  const box = $("#pending");
  box.replaceChildren();
  for (const p of inventory.pending || []) {
    const card = node("div", undefined, "pending");
    card.append(
      node("strong", p.friendly),
      node("small", p.installed
        ? "Installed, but not yet in Home Assistant. That happens outside ESP Screens: add the discovered ESPHome device under Settings → Devices & services, paste the API key there, and turn on “Allow the device to perform Home Assistant actions” under Configure."
        : `Not yet in Home Assistant. Already flashed? Add the ESPHome device under Settings → Devices & services, then allow the Home Assistant actions under Configure. Not flashed yet? Settings → Firmware & USB → ${p.file}.`),
    );
    const actions = node("div", undefined, "pending-actions");
    const go = node("button", "Open Devices & services", "mini");
    go.type = "button";
    go.onclick = openIntegrations;
    actions.append(go);
    if (p.api_key) {
      const copy = node("button", "Copy API key", "mini quiet");
      copy.type = "button";
      copy.onclick = () => copyText(p.api_key);
      actions.append(copy);
    }
    card.append(actions);
    box.append(card);
  }
}
function renderScreens() {
  renderPending();
  // Keep an open address form alive across the periodic refresh.
  if ($("#screens .screen-host")) {
    renderUpdates();
    return;
  }
  $("#screens").replaceChildren();
  for (const screen of inventory.screens) {
    if (screen.update?.state === "running") updating.delete(screen.id);
    const item = node(
      "div",
      undefined,
      `screen ${screen.id === selected ? "selected" : ""}`,
    );
    const b = node("button", undefined, "screen-main");
    b.type = "button";
    const meta = node("small");
    meta.append(
      node("span", screen.online ? "●" : "○", `dot ${screen.online ? "online" : ""}`),
      ` ${screen.online ? "Online" : "Offline"}${screen.area ? " · " + screen.area : ""} · firmware ${screen.firmware || "unknown"}`,
    );
    b.append(node("strong", screen.name), meta);
    b.onclick = () => select(screen.id);
    item.append(b);
    const override = node("button", "Override YAML", "mini quiet");
    override.type = "button";
    override.disabled = !screen.update?.profile;
    override.title = screen.update?.profile ? "Hardware-specific YAML for this screen" : "No ESPHome profile found";
    override.onclick = (event) => {
      event.stopPropagation();
      openOverride(screen.update?.profile, screen.name);
    };
    const overrideRow = node("div", undefined, "screen-override");
    overrideRow.append(override);
    item.append(overrideRow);
    const update = renderUpdate(screen);
    if (update) item.append(update);
    $("#screens").append(item);
  }
  const screen = inventory.screens.find((s) => s.id === selected);
  if (screen) {
    $("#delivery").textContent = screen.online
      ? `${screen.delivery} · ${screen.status}`
      : "Offline · changes are saved";
    $("#delivery").classList.toggle("online", screen.online);
  }
  renderUpdates();
}

// ----- Per-screen local YAML overrides -----
const OVERRIDE_EXAMPLE = `# Hardware-specific changes for this screen.
# This file is kept when the shared firmware package updates.
# Do not add esphome:, api:, ota:, wifi: or packages: here.

# Example: a CYD with the ST7789V display controller.
# "!extend" changes the display the shared package already defines;
# a bare id would add a second, incomplete display and fail the build.
display:
  - id: !extend my_display
    model: ST7789V
`;
const overrideEditor = $("#override-editor"), overrideGutter = $("#override-gutter");
let overrideProfile = null;
function updateOverrideEditor() {
  if (!overrideEditor || !overrideGutter) return;
  const lines = overrideEditor.value.split("\n").length;
  overrideGutter.textContent = Array.from({ length: lines }, (_, i) => i + 1).join("\n");
  $("#override-count").textContent = `${overrideEditor.value.length} characters · ${lines} line${lines === 1 ? "" : "s"}`;
}
function setOverrideStatus(message, kind = "") {
  const status = $("#override-status");
  status.textContent = message;
  status.className = `override-status ${kind}`;
}
function setOverrideContent(content) {
  overrideEditor.value = content || "";
  updateOverrideEditor();
  overrideEditor.scrollTop = 0;
  overrideGutter.scrollTop = 0;
}
async function openOverride(profile, friendly = "") {
  if (!profile) {
    toast("No ESPHome profile was found for this screen.");
    return;
  }
  overrideProfile = profile;
  $("#override-title").textContent = `Override YAML${friendly ? ` · ${friendly}` : ""}`;
  setOverrideStatus("Loading…");
  $("#override-dialog").showModal();
  try {
    const data = await (await api(`firmware/profiles/${encodeURIComponent(profile)}/override`)).json();
    $("#override-file").textContent = data.override_file;
    $("#override-state").textContent = data.attached ? "Active" : "Ready to attach";
    $("#override-state").className = `badge${data.attached ? " online" : ""}`;
    setOverrideContent(data.exists && data.content !== "{}\n" ? data.content : "");
    setOverrideStatus(data.attached ? "Changes here apply on the next build." : "Save once to attach this file to the profile.");
  } catch (error) {
    setOverrideStatus(error.message, "error");
  }
}
function overridePayload() {
  return { content: overrideEditor.value };
}
async function saveOverride(runCheck = false) {
  if (!overrideProfile) return;
  const save = $("#override-save"), check = $("#override-check");
  save.disabled = true; check.disabled = true;
  setOverrideStatus("Checking and saving…");
  try {
    const response = await api(`firmware/profiles/${encodeURIComponent(overrideProfile)}/override`, {
      method: "PUT", body: JSON.stringify(overridePayload()),
    });
    const data = await response.json();
    $("#override-file").textContent = data.override_file;
    $("#override-state").textContent = "Active";
    $("#override-state").className = "badge online";
    if (!runCheck) {
      setOverrideStatus("Saved. The override is kept during firmware updates.", "ok");
      toast("Override YAML saved.");
      return;
    }
    setOverrideStatus("Saved. ESPHome is checking the complete profile…");
    const job = await (await api("firmware/jobs", {
      method: "POST", body: JSON.stringify({ file: overrideProfile, action: "validate" }),
    })).json();
    pollOverrideCheck(job);
  } catch (error) {
    setOverrideStatus(error.message, "error");
  } finally {
    save.disabled = false; check.disabled = false;
  }
}
function pollOverrideCheck(job) {
  const started = Date.now(), profile = overrideProfile;
  const poll = async () => {
    if (overrideProfile !== profile) return;  // the dialog closed or shows another screen
    try {
      const data = await (await api("firmware")).json();
      const current = data.job;
      if (current && current.file === profile && current.state === "running") {
        setOverrideStatus(`ESPHome is checking the profile… ${current.stage || ""}`.trim());
      } else if (current && current.file === profile && current.state === "success") {
        setOverrideStatus("The complete profile is valid. Safe to build and install.", "ok");
        return;
      } else if (current && current.file === profile && current.state === "failed") {
        const error = (data.logs || []).filter((line) => /error|failed/i.test(line)).pop();
        setOverrideStatus(error || "ESPHome rejected the complete profile. See Firmware & USB for the full log.", "error");
        return;
      }
      if (Date.now() - started < 7200000) setTimeout(poll, 1200);
    } catch (error) {
      setOverrideStatus(error.message, "error");
    }
  };
  poll();
}
$("#open-override").onclick = () => {
  const screen = inventory.screens.find((entry) => entry.id === selected);
  openOverride(screen?.update?.profile, screen?.name);
};
$("#close-override").onclick = () => $("#override-dialog").close();
$("#override-dialog").addEventListener("close", () => { overrideProfile = null; });
$("#override-save").onclick = () => saveOverride(false);
$("#override-check").onclick = () => saveOverride(true);
$("#override-example").onclick = () => {
  if (!overrideEditor.value.trim() || confirm("Replace the current text with the example?")) {
    setOverrideContent(OVERRIDE_EXAMPLE);
    setOverrideStatus("Example loaded. Save it when you are ready.");
    overrideEditor.focus();
  }
};
$("#override-empty").onclick = () => {
  if (!overrideEditor.value.trim() || confirm("Clear the local override?")) {
    setOverrideContent("");
    setOverrideStatus("The override will be empty after you save.");
    overrideEditor.focus();
  }
};
overrideEditor.addEventListener("input", updateOverrideEditor);
overrideEditor.addEventListener("scroll", () => { overrideGutter.scrollTop = overrideEditor.scrollTop; });
overrideEditor.addEventListener("keydown", (event) => {
  if (event.key === "Tab") {
    event.preventDefault();
    const start = overrideEditor.selectionStart, end = overrideEditor.selectionEnd;
    overrideEditor.setRangeText("  ", start, end, "end");
    updateOverrideEditor();
  } else if ((event.ctrlKey || event.metaKey) && event.key.toLowerCase() === "s") {
    event.preventDefault();
    saveOverride(false);
  }
});
updateOverrideEditor();
function renderUpdates() {
  const u = inventory.updates;
  $("#updates").hidden = !u;
  if (!u) return;
  const outdated = inventory.screens.filter((s) => s.update?.available).length;
  $("#updates-hint").textContent = u.busy
    ? `Updating to firmware ${u.target}…`
    : !inventory.screens.length
      ? `No screen paired yet. New screens get firmware ${u.target}.`
      : outdated
        ? `Firmware ${u.target} is available for ${outdated} screen${outdated === 1 ? "" : "s"}.`
        : `All screens have firmware ${u.target}.`;
  $("#update-all").hidden = !u.pending || !!u.busy || u.pending < 2;
  $("#update-all").textContent = `Update all ${u.pending} screens`;
  if (document.activeElement !== $("#auto-update"))
    $("#auto-update").checked = u.auto;
}
$("#update-all").onclick = async () => {
  try {
    await api("updates/run", { method: "POST" });
    await refresh();
  } catch (e) {
    toast(e.message);
  }
};
// ----- Settings: a view of its own next to My screens; #settings keeps it open across a reload -----
function showView() {
  const settings = location.hash === "#settings";
  $("#home-view").hidden = settings;
  $("#settings-view").hidden = !settings;
  $("#open-settings-label").textContent = settings ? "My screens" : "Settings";
  $("#open-settings-icon").textContent = settings ? "▦" : "⚙";
  $("#open-settings-hint").textContent = settings ? "Tiles and top bar" : "New screen, updates, alerts, Claude";
  $("#open-settings").setAttribute("aria-expanded", String(settings));
  if (settings) {
    renderUpdates();
    renderClaude();
  }
  window.scrollTo(0, 0);
}
$("#open-settings").onclick = () => {
  location.hash = location.hash === "#settings" ? "" : "#settings";
};
$("#close-settings").onclick = () => {
  location.hash = "";
};
window.addEventListener("hashchange", showView);
showView();
function renderClaude() {
  const skill = inventory.claude_skill, button = $("#claude-install"), status = $("#claude-status");
  button.disabled = !skill;
  if (!skill) {
    status.textContent = "Loading…";
    return;
  }
  $("#claude-path").textContent = skill.path;
  button.textContent = !skill.installed ? "Install for Claude Code" : skill.current ? "Install again" : "Update the skill";
  button.classList.toggle("again", skill.installed && skill.current);
  status.textContent = !skill.installed ? "Not installed" : skill.current ? "● Installed" : "Installed · a newer version is ready";
  status.className = !skill.installed ? "hint" : skill.current ? "badge online" : "badge update";
}
$("#claude-install").onclick = async () => {
  const button = $("#claude-install");
  button.disabled = true;
  try {
    inventory.claude_skill = await (await api("claude-skill", { method: "POST" })).json();
    toast(inventory.claude_skill.restart
      ? "Skill installed. Restart Claude Code once so it finds the new skills folder."
      : "Skill installed. Claude Code picks it up right away.");
  } catch (e) {
    toast(e.message);
  } finally {
    renderClaude();
  }
};
$("#auto-update").onchange = async () => {
  try {
    await api("updates", {
      method: "PUT",
      body: JSON.stringify({ auto: $("#auto-update").checked }),
    });
    toast(
      $("#auto-update").checked
        ? "Screens will now update automatically at night."
        : "Automatic updates are off.",
    );
  } catch (e) {
    $("#auto-update").checked = !$("#auto-update").checked;
    toast(e.message);
  }
};
const domains = {
  light: ["Light", "☀", "#ad7600", "#fff3d3"],
  climate: ["Climate", "❄", "#c86620", "#ffebdc"],
  vacuum: ["Vacuum", "◉", "#008577", "#def3ed"],
  fan: ["Fan", "✣", "#008aab", "#def5fa"],
  cover: ["Cover", "▤", "#8053af", "#eee5f8"],
  media_player: ["Media", "▶", "#007cad", "#def2fc"],
  sensor: ["Sensor", "⌁", "#3476b1", "#e5effa"],
  binary_sensor: ["Status", "◈", "#ad7600", "#fff3d3"],
  switch: ["Switch", "⏻", "#ad7600", "#fff3d3"],
  input_boolean: ["Switch", "⏻", "#ad7600", "#fff3d3"],
  scene: ["Scene", "✦", "#8053af", "#eee5f8"],
  script: ["Script", "▷", "#8053af", "#eee5f8"],
  weather: ["Weather", "☁", "#007cad", "#def2fc"],
  number: ["Value", "±", "#008577", "#def3ed"],
  input_number: ["Value", "±", "#008577", "#def3ed"],
  select: ["Select", "≡", "#5862af", "#eaecfa"],
  input_select: ["Select", "≡", "#5862af", "#eaecfa"],
  button: ["Action", "↗", "#5862af", "#eaecfa"],
  screen: ["Clock", "◷", "#25282c", "#e9ecf1"],
  sun: ["Sun", "☼", "#c86620", "#ffebdc"],
  timer: ["Timer", "⏱", "#008577", "#def3ed"],
  person: ["Person", "☺", "#2f7d32", "#e1f2e2"],
};
const displayNames = { standard: "standard", watch: "large value", forecast: "weather forecast", graph: "graph", digital: "digital clock", analog: "analog clock", sunpath: "sun path" };
// Same rule as the add-on: only a wide card in the standard layout shows direct controls;
// without a choice the domain's first control set applies.
function effectiveControls(tile) {
  const domain = tile.entity.split(".")[0], catalogue = inventory.controls?.[domain], o = tile.options || {};
  if (!catalogue || o.size !== "wide" || (o.display || "standard") !== "standard" || o.inline === "slider") return null;
  const choice = o.controls ?? catalogue.default;
  return choice === "none" ? null : choice;
}
function controlsLabel(tile) {
  const key = effectiveControls(tile);
  if (!key) return "none";
  return inventory.controls?.[tile.entity.split(".")[0]]?.choices.find((c) => c.key === key)?.label.toLocaleLowerCase() || key;
}
// Miniature of the control set on the mockup card: the same shapes the screen draws.
function controlsPreview(tile) {
  const key = effectiveControls(tile), domain = tile.entity.split(".")[0], cp = inventory.icons?.controls || {};
  if (!key) return null;
  const box = node("span", undefined, "preview-controls");
  const buttons = (...names) => { for (const name of names) box.append(node("span", cp[name] ? glyph(cp[name]) : "", "preview-key mdi")); };
  const stepper = (text) => { const pill = node("span", undefined, "preview-stepper"); pill.append(node("span", cp.minus ? glyph(cp.minus) : "−", "mdi"), node("b", text), node("span", cp.plus ? glyph(cp.plus) : "+", "mdi")); box.append(pill); };
  const slider = () => box.append(node("span", "", "preview-range"));
  if (key === "toggle") box.append(node("span", "", "preview-toggle"));
  else if (key === "setpoint") stepper("20°");
  else if (key === "stepper") domain.endsWith("select") ? buttons("chevron-left", "chevron-right") : stepper("50");
  else if (key === "mode") buttons("power", "fire", "snowflake");
  else if (key === "volume") { slider(); buttons("volume-high"); }
  else if (key === "playback") buttons("skip-previous", "play", "skip-next");
  else if (key === "buttons" && domain === "cover") buttons("arrow-expand-horizontal", "stop", "arrow-collapse-horizontal");
  else if (key === "buttons" && domain === "vacuum") buttons("play", "stop", "home-map-marker");
  else if (key === "buttons" && domain === "timer") buttons("play", "close");
  else if (key === "run") box.append(node("b", { scene: "Activate", script: "Run" }[domain] || "Press", "preview-run"));
  else slider();
  return box;
}
// New tiles start with the card that shows the entity best.
function defaultOptions(id) {
  const domain = id.split(".")[0];
  if (domain === "sun") return { options: { display: "sunpath", size: "wide" } };
  if (domain === "weather") return { options: { display: "forecast", size: "wide" } };
  if (domain === "screen") return { options: { display: "digital", size: "wide" } };
  return {};
}
// ---- Grid positions ----
// Two columns, three rows per page, at most eight pages. A tile's `slot` is its absolute
// cell (page * 6 + row * 2 + column); a wide tile starts in the left column and also covers
// the cell to its right. Empty cells are allowed and stay exactly where they are.
const SLOTS_PER_PAGE = 6, MAX_PAGES = 8, MAX_SLOTS = MAX_PAGES * SLOTS_PER_PAGE;
const isWide = (tile) => tile.options?.size === "wide";
const cellsOf = (slot, wide) => (wide ? [slot, slot + 1] : [slot]);
const rowStart = (slot) => slot - (slot % 2);
const pageOf = (slot) => Math.floor(slot / SLOTS_PER_PAGE);
const liveEntries = () => layout.tiles.map((tile) => ({ tile, slot: tile.slot }));
// In-order packing: the rule before positions existed, and what firmware below 0.2.26 still draws.
function packSlots(tiles) {
  let position = 0;
  return tiles.map((tile) => {
    const wide = isWide(tile);
    if (wide && position % 2 === 1) position++;
    const slot = position;
    position += wide ? 2 : 1;
    return slot;
  });
}
function hasGaps(tiles) {
  const packed = packSlots(tiles);
  return tiles.some((tile, i) => tile.slot !== packed[i]);
}
// Every tile gets a position (older layouts pack in order), the list stays in reading
// order and the open settings sheet keeps following its tile.
function normalize() {
  if (layout.tiles.some((t) => !Number.isInteger(t.slot))) { const packed = packSlots(layout.tiles); layout.tiles.forEach((t, i) => (t.slot = packed[i])); }
  layout.tiles.sort((a, b) => a.slot - b.slot);
  if (sheetIndex >= 0) sheetIndex = layout.tiles.findIndex((t) => t.entity === selectedTile);
}
function occupied(entries) {
  const taken = new Set();
  for (const { tile, slot } of entries) for (const cell of cellsOf(slot, isWide(tile))) taken.add(cell);
  return taken;
}
const fits = (taken, slot, wide) => Number.isInteger(slot) && slot >= 0 && slot + (wide ? 1 : 0) < MAX_SLOTS && !(wide && slot % 2) && cellsOf(slot, wide).every((c) => !taken.has(c));
function firstFree(taken, wide) {
  for (let slot = 0; slot < MAX_SLOTS; slot++) if (fits(taken, slot, wide)) return slot;
  return -1;
}
// The free position closest to `origin`; on a tie the later one, so a nudged tile moves down, not up.
function nearestFree(taken, wide, origin) {
  let best = -1;
  for (let slot = 0; slot < MAX_SLOTS; slot++) if (fits(taken, slot, wide) && (best < 0 || Math.abs(slot - origin) <= Math.abs(best - origin))) best = slot;
  return best;
}
// The arrangement after putting `moving` (a tile on the grid, or a new one) at `target`:
// it lands exactly there; tiles in its way take the cells it left (a swap) or else the
// nearest free cell; everything else stays put. Null when the target is off the grid.
function arrange(tiles, moving, target) {
  const wide = isWide(moving);
  if (wide) target = rowStart(target);
  if (!fits(new Set(), target, wide)) return null;
  const footprint = cellsOf(target, wide);
  const vacated = tiles.includes(moving) ? cellsOf(moving.slot, wide) : [];
  const result = [{ tile: moving, slot: target }], displaced = [];
  for (const tile of tiles) {
    if (tile === moving) continue;
    if (cellsOf(tile.slot, isWide(tile)).some((c) => footprint.includes(c))) displaced.push(tile);
    else result.push({ tile, slot: tile.slot });
  }
  for (const tile of displaced) {
    const w = isWide(tile), taken = occupied(result);
    let slot = vacated.map((c) => (w ? rowStart(c) : c)).find((c) => fits(taken, c, w));
    if (slot === undefined) slot = nearestFree(taken, w, tile.slot);
    if (slot < 0) return null;
    result.push({ tile, slot });
  }
  return result.sort((a, b) => a.slot - b.slot);
}
// Apply an arrangement; a new tile joins the layout. True when anything changed.
function commit(result) {
  const before = layout.tiles.map((t) => `${t.entity}@${t.slot}`).join();
  for (const { tile, slot } of result) { tile.slot = slot; if (!layout.tiles.includes(tile)) layout.tiles.push(tile); }
  normalize();
  const changed = layout.tiles.map((t) => `${t.entity}@${t.slot}`).join() !== before;
  if (changed) { markDirty(); renderTiles(); renderResults(); }
  return changed;
}
function placeTile(tile, target) {
  const result = arrange(layout.tiles, tile, target);
  return result ? commit(result) : false;
}
// Pages the tiles need, or more when the user keeps empty pages on purpose (`layout.pages`).
function pageCount(entries) {
  const last = Math.max(0, ...entries.map(({ tile, slot }) => slot + (isWide(tile) ? 2 : 1)));
  return Math.min(MAX_PAGES, Math.max(1, Math.ceil(last / SLOTS_PER_PAGE), layout.pages || 1));
}
function addPage() {
  layout.pages = Math.min(MAX_PAGES, pageCount(liveEntries()) + 1);
  markDirty();
  renderTiles();
  document.querySelector("#layout-preview .screen-preview:last-child")?.scrollIntoView({ behavior: "smooth", block: "nearest" });
}
// An empty page goes; the pages after it move up.
function removePage(page) {
  for (const tile of layout.tiles) if (pageOf(tile.slot) > page) tile.slot -= SLOTS_PER_PAGE;
  layout.pages = Math.max(1, pageCount(liveEntries()) - 1);
  markDirty();
  renderTiles();
}
function focusSlot(slot) {
  document.querySelector(`#layout-preview [data-slot="${slot}"]`)?.focus();
}
function supportsFirmware(major, minor, patch) {
  const version = inventory.screens.find((s) => s.id === selected)?.firmware || "";
  const match = /^(\d+)\.(\d+)\.(\d+)$/.exec(version);
  if (!match) return false;
  const [a, b, c] = match.slice(1).map(Number);
  return a > major || (a === major && (b > minor || (b === minor && c >= patch)));
}
function domainBadge(id) {
  const [title, symbol, color, background] = domains[id.split(".")[0]] || ["Entity", "◇", "#637184", "#edf0f4"];
  const badge = node("span", symbol, "domain-icon");
  badge.title = title;
  badge.setAttribute("aria-label", title);
  badge.style.color = color;
  badge.style.background = background;
  return badge;
}
// The screen draws Material Design Icons; static/tile-icons.woff holds the same glyphs.
const glyph = (cp) => String.fromCodePoint(parseInt(cp, 16));
let iconIndex = { source: null, byName: {} };
function iconNamed(name) {
  if (iconIndex.source !== inventory.icons)
    iconIndex = { source: inventory.icons, byName: Object.fromEntries((inventory.icons?.groups || []).flatMap((g) => g.icons.map((i) => [i.name, i]))) };
  return iconIndex.byName[name];
}
// What the firmware draws without a choice: Home Assistant's own icon, else the domain icon.
function automaticIcon(id) {
  const icons = inventory.icons, entity = inventory.entities.find((e) => e.id === id), domain = id.split(".")[0];
  if (icons.builtin?.[id]) return icons.builtin[id];
  if (entity?.icon) return entity.icon;
  if (domain === "weather") return icons.weather[entity?.state] || icons.weather.partlycloudy;
  if (domain === "sun") return icons.sun[entity?.state] || icons.sun.below_horizon;
  return icons.defaults[domain] || icons.fallback;
}
function tileBadge(tile) {
  const badge = domainBadge(tile.entity);
  if (!inventory.icons) return badge;
  badge.textContent = glyph(iconNamed(tile.options?.icon)?.cp || automaticIcon(tile.entity));
  badge.classList.add("mdi");
  return badge;
}
function tileLimit() {
  const version=inventory.screens.find(s=>s.id===selected)?.firmware || "";
  const match=/^(\d+)\.(\d+)\.(\d+)$/.exec(version);
  if(!match)return 10;
  return Number(match[1])>0 || Number(match[2])>2 || Number(match[2])===2 && Number(match[3])>=7 ? 20 : 10;
}
function entityName(id) {
  return inventory.entities.find((e) => e.id === id)?.name || inventory.builtin?.find((e) => e.id === id)?.name || id;
}
let insertAt = -1;  // empty cell chosen as the place for the next tile from the picker
function renderPreview(preview) {
  const root = $("#layout-preview");
  root.replaceChildren();
  // While dragging, the mockup already shows the arrangement after the drop.
  const entries = preview?.result || liveEntries();
  const moving = preview?.moving || null;
  const bySlot = new Map(entries.map((e) => [e.slot, e]));
  const covered = new Set(entries.filter((e) => isWide(e.tile)).map((e) => e.slot + 1));
  const pages = pageCount(entries);
  // While dragging, one more page waits below the last one.
  const shown = drag.active && pages < MAX_PAGES ? pages + 1 : pages;
  for (let page = 0; page < shown; page++) {
    const frame = node("section", undefined, "screen-preview");
    const heading = node("div", undefined, "preview-heading");
    heading.append(node("small", `Page ${page + 1}`));
    if (page >= pages) {
      frame.classList.add("new-page");
      heading.replaceChildren(node("small", `Page ${page + 1} · drag here for a new page`));
    } else if (pages > 1 && !entries.some((e) => pageOf(e.slot) === page)) {
      heading.append(node("small", "empty", "page-note"));
      const drop = node("button", "Remove page", "mini");
      drop.type = "button";
      drop.title = "The pages after this one shift up one slot";
      drop.onclick = () => removePage(page);
      heading.append(drop);
    }
    frame.append(heading);
    if (page < pages) frame.append(topbarBar());
    const grid = node("div", undefined, "preview-grid");
    for (let cell = 0; cell < SLOTS_PER_PAGE; cell++) {
      const slot = page * SLOTS_PER_PAGE + cell;
      if (covered.has(slot)) continue;
      const entry = bySlot.get(slot);
      grid.append(entry ? tileCard(entry.tile, slot, entry.tile === moving) : emptyCell(slot));
    }
    frame.append(grid);
    root.append(frame);
  }
  fitTopbars();
}
// A card on the mockup. A placeholder is the tile being dragged, drawn where it will land.
function tileCard(tile, slot, placeholder) {
  const index = layout.tiles.indexOf(tile);
  const card = node("div", undefined, "preview-tile");
  card.dataset.slot = slot;
  const name = tile.name || entityName(tile.entity);
  const background = inventory.backgrounds?.[tile.options?.background]?.color;
  if (background) card.style.backgroundColor = background;
  // "None": no card on the screen; the mockup keeps a dashed outline.
  if (tile.options?.background === "none") card.classList.add("bare");
  if (isWide(tile)) card.classList.add("wide");
  card.append(tileBadge(tile), node("strong", name));
  if (tile.options?.inline === "slider") card.append(node("span", "", "preview-slider"));
  const controls = controlsPreview(tile);
  if (controls) card.append(controls);
  const display = tile.options?.display;
  if (display && display !== "standard") card.append(node("small", displayNames[display] || display));
  if (placeholder || index < 0) { card.classList.add("placeholder"); return card; }
  card.tabIndex = 0;
  card.setAttribute("role", "button");
  card.setAttribute("aria-label", `${name}, slot ${(slot % SLOTS_PER_PAGE) + 1} on page ${pageOf(slot) + 1}. Enter: configure, arrow keys: move`);
  card.classList.toggle("chosen", selectedTile === tile.entity);
  enableDrag(card, { kind: "tile", tile });
  card.onclick = () => openTileSheet(index);
  card.onkeydown = (e) => {
    if (e.key === "Enter" || e.key === " ") { e.preventDefault(); openTileSheet(index); return; }
    const step = { ArrowLeft: -1, ArrowRight: 1, ArrowUp: -2, ArrowDown: 2 }[e.key];
    if (!step) return;
    e.preventDefault();
    // A wide card owns its row: left and right mean the row above and below.
    if (placeTile(tile, tile.slot + (isWide(tile) ? Math.sign(step) * 2 : step))) focusSlot(tile.slot);
  };
  const remove = node("button", "✕", "preview-remove");
  remove.type = "button";
  remove.title = "Remove tile";
  remove.setAttribute("aria-label", `Remove ${name}`);
  remove.onclick = (e) => { e.stopPropagation(); removeTile(index); };
  card.append(remove);
  return card;
}
// An empty cell: a drop target, and a click marks it as the place for the next tile.
function emptyCell(slot) {
  const cell = node("button", undefined, "preview-cell");
  cell.type = "button";
  cell.dataset.slot = slot;
  const marked = insertAt === slot;
  cell.classList.toggle("insert-here", marked);
  cell.append(node("span", "+"), node("small", marked ? "Next tile goes here" : "Empty"));
  cell.title = "Empty slot. Click to add a tile here, or drag one over.";
  cell.setAttribute("aria-label", `Empty slot ${(slot % SLOTS_PER_PAGE) + 1} on page ${pageOf(slot) + 1}: add the next tile here`);
  cell.onclick = () => {
    insertAt = marked ? -1 : slot;
    renderPreview();
    if (insertAt >= 0) { $("#search").focus(); $("#search").scrollIntoView({ behavior: "smooth", block: "center" }); }
  };
  return cell;
}
function renderTiles() {
  normalize();
  // Pages follow the tiles; only a page the user added on purpose can stay empty.
  layout.pages = pageCount(liveEntries());
  renderPreview();
  $("#add-page").disabled = layout.pages >= MAX_PAGES;
  $("#count").textContent = `${layout.tiles.length} / ${tileLimit()}${tileLimit()===10?" · update firmware for 20":""}`;
  $("#no-tiles").hidden = layout.tiles.length > 0;
  // Firmware below 0.2.26 ignores positions and packs the tiles in order, without gaps.
  const hint = $("#positions-hint");
  hint.hidden = !hasGaps(layout.tiles) || supportsFirmware(0, 2, 26);
  hint.textContent = `Empty slots and fixed positions work from firmware 0.2.26. This screen (firmware ${inventory.screens.find((s) => s.id === selected)?.firmware || "unknown"}) shifts the tiles up to the first free slot until that update.`;
  if (sheetIndex >= 0) renderTileSheet();
}
function removeTile(index) {
  const [tile] = layout.tiles.splice(index, 1);
  if (!tile) return;
  if (sheetIndex >= 0) closeTileSheet();
  markDirty();
  renderTiles();
  renderResults();
  toast(`${tile.name || entityName(tile.entity)} removed`, {
    label: "Undo",
    run: () => placeTile(tile, tile.slot),
  });
}
// Tile settings open in a sheet above the mockup; every change applies live,
// so the card behind it shows the result while you pick.
let sheetIndex = -1, iconPickerOpen = false;
function openTileSheet(index) {
  if (index !== sheetIndex) iconPickerOpen = false;
  sheetIndex = index;
  selectedTile = layout.tiles[index].entity;
  renderTileSheet();
  renderPreview();
  const sheet = $("#tile-sheet");
  if (!sheet.open) sheet.showModal();
}
function closeTileSheet() {
  sheetIndex = -1;
  selectedTile = null;
  const sheet = $("#tile-sheet");
  if (sheet.open) sheet.close();
  renderPreview();
}
$("#tile-sheet").addEventListener("close", () => { if (sheetIndex >= 0) { sheetIndex = -1; selectedTile = null; renderPreview(); } });
function field(title, control) {
  const label = node("label", undefined, "sheet-field");
  label.append(node("span", title), control);
  return label;
}
function segmented(choices, value, onChange) {
  const group = node("div", undefined, "segmented");
  for (const [key, text] of choices) {
    const b = node("button", text);
    b.type = "button";
    b.setAttribute("aria-pressed", String(String(key) === String(value)));
    b.onclick = () => { for (const other of group.children) other.setAttribute("aria-pressed", String(other === b)); onChange(key); };
    group.append(b);
  }
  return group;
}
// A choice applies live like the palette: only pressed states, the summary and the
// badges update, so search text and scroll position survive.
function iconField(tile, onChange) {
  const fromHA = Boolean(inventory.entities.find((e) => e.id === tile.entity)?.icon);
  return iconPicker({
    selected: tile.options?.icon || "auto",
    automatic: automaticIcon(tile.entity),
    autoLabel: `Automatic (${fromHA ? "from Home Assistant" : "default"})`,
    onPick: (name) => { tile.options = { ...tile.options, icon: name }; markDirty(); renderPreview(); onChange(); },
    note: supportsFirmware(0, 2, 18) ? "" : "The screen shows a chosen icon from firmware 0.2.18.",
  });
}
// The icon choice for tiles and top bar items: automatic, optionally none, or one from the set.
function iconPicker({ selected, automatic, autoLabel, allowNone = false, onPick, note = "" }) {
  const wrap = node("div", undefined, "sheet-field");
  wrap.append(node("span", "Icon"));
  let picked = selected;
  const summary = node("button", undefined, "icon-current");
  summary.type = "button";
  summary.setAttribute("aria-expanded", String(iconPickerOpen));
  const current = node("span", undefined, "mdi"), text = node("span"), action = node("small", iconPickerOpen ? "Close" : "Change");
  summary.append(current, text, action);
  const describe = () => {
    const chosen = iconNamed(picked);
    current.textContent = picked === "none" ? "" : glyph(chosen?.cp || automatic);
    text.textContent = picked === "none" ? "No icon" : chosen?.label || autoLabel;
  };
  describe();
  const panel = node("div", undefined, "icon-picker");
  panel.hidden = !iconPickerOpen;
  summary.onclick = () => {
    iconPickerOpen = panel.hidden;
    panel.hidden = !iconPickerOpen;
    summary.setAttribute("aria-expanded", String(iconPickerOpen));
    action.textContent = iconPickerOpen ? "Close" : "Change";
  };
  const choice = (name, cp, label, cls = "") => {
    const b = node("button", undefined, `icon-choice ${cls}`);
    b.type = "button";
    b.dataset.icon = name;
    b.dataset.search = label.toLocaleLowerCase();
    b.title = label;
    b.setAttribute("aria-label", `Icon: ${label}`);
    b.setAttribute("aria-pressed", String(picked === name));
    b.append(node("span", cp ? glyph(cp) : "", "mdi"));
    b.onclick = () => {
      picked = name;
      for (const other of panel.querySelectorAll(".icon-choice")) other.setAttribute("aria-pressed", String(other === b));
      describe();
      onPick(name);
    };
    return b;
  };
  const search = node("input");
  search.type = "search";
  search.placeholder = "Search, for example lamp, music, or door";
  search.setAttribute("aria-label", "Search for an icon");
  const auto = choice("auto", automatic, autoLabel, "icon-auto");
  auto.append(node("span", autoLabel));
  const none = allowNone ? choice("none", "", "No icon", "icon-auto icon-none") : null;
  none?.append(node("span", "No icon, text only"));
  const list = node("div", undefined, "icon-list"), empty = node("p", "No icon found.", "hint");
  empty.hidden = true;
  const sections = inventory.icons.groups.map((group) => {
    const section = node("section"), grid = node("div", undefined, "icon-grid");
    for (const icon of group.icons) {
      const b = choice(icon.name, icon.cp, icon.label);
      b.dataset.search += ` ${icon.name.replaceAll("-", " ")} ${group.label.toLocaleLowerCase()}`;
      grid.append(b);
    }
    section.append(node("small", group.label), grid);
    list.append(section);
    return section;
  });
  search.oninput = () => {
    const query = search.value.trim().toLocaleLowerCase();
    let found = 0;
    for (const section of sections) {
      let visible = 0;
      for (const b of section.querySelectorAll(".icon-choice")) {
        b.hidden = !b.dataset.search.includes(query);
        if (!b.hidden) visible++;
      }
      section.hidden = !visible;
      found += visible;
    }
    empty.hidden = found > 0;
  };
  panel.append(search, auto, ...(none ? [none] : []), list, empty);
  wrap.append(summary, panel);
  if (note) wrap.append(node("small", note, "icon-hint"));
  return wrap;
}
function renderTileSheet() {
  const sheet = $("#tile-sheet"), tile = layout.tiles[sheetIndex];
  if (!tile) return;
  sheet.replaceChildren();
  const domain = tile.entity.split(".")[0], name = entityName(tile.entity);
  const head = node("div", undefined, "sheet-head"), titles = node("div");
  titles.append(node("strong", name), node("small", tile.entity));
  const close = node("button", "✕", "quiet sheet-close");
  close.type = "button";
  close.setAttribute("aria-label", "Close");
  close.onclick = closeTileSheet;
  let badge = tileBadge(tile);
  head.append(badge, titles, close);
  const body = node("div", undefined, "sheet-body");
  const nameInput = node("input");
  nameInput.value = tile.name;
  nameInput.placeholder = name;
  nameInput.maxLength = 60;
  nameInput.oninput = () => { tile.name = nameInput.value; markDirty(); renderPreview(); };
  body.append(field("Name on the screen", nameInput));
  // The clock, forecast and sun path cards draw no tile icon.
  if (inventory.icons && domain !== "screen" && !["forecast", "sunpath"].includes(tile.options?.display))
    body.append(iconField(tile, () => { const next = tileBadge(tile); badge.replaceWith(next); badge = next; }));
  const displays = domain === "screen"
    ? [["digital", "Digital clock"], ["analog", "Analog clock"]]
    : [["standard", "Name and status"], ["watch", "Large value"]];
  if (domain === "weather") displays.push(["forecast", "Weather forecast"]);
  if (domain === "sensor") displays.push(["graph", "Graph"]);
  if (domain === "sun") displays.push(["sunpath", "Sun path"]);
  const current = (key, fallback) => tile.options?.[key] ?? fallback;
  const set = (key, value) => {
    const wasWide = isWide(tile);
    tile.options = { ...tile.options, [key]: value };
    // Direct controls need the standard layout without a mini slider, and vice versa.
    if (key === "display" && value === "watch") { tile.options.inline = "none"; if (inventory.controls?.[domain]) tile.options.controls = "none"; }
    if (key === "display" && ["forecast", "sunpath"].includes(value)) tile.options.size = "wide";
    if (key === "inline" && value === "slider") { tile.options.display = "standard"; if (inventory.controls?.[domain]) tile.options.controls = "none"; }
    if (key === "controls" && value !== "none") { tile.options.display = "standard"; tile.options.inline = "none"; }
    markDirty();
    // A card that becomes double-wide keeps its row when the cell beside it is free, else it
    // takes the nearest free row (below first); every other tile stays where it is.
    if (isWide(tile) && !wasWide) {
      const taken = occupied(liveEntries().filter((e) => e.tile !== tile)), own = rowStart(tile.slot);
      const slot = fits(taken, own, true) ? own : nearestFree(taken, true, own);
      if (slot >= 0) tile.slot = slot;
    }
    renderTiles();
  };
  body.append(field("Display", segmented(displays, current("display", domain === "screen" ? "digital" : "standard"), (v) => set("display", v))));
  body.append(field("Width", segmented([["single", "Normal"], ["wide", "Double-width"]], current("size", "single"), (v) => set("size", v))));
  const catalogue = inventory.controls?.[domain];
  if (catalogue && current("size", "single") === "wide") {
    const wrap = field("Direct control on the tile", segmented(catalogue.choices.map((c) => [c.key, c.label]), current("controls", catalogue.default), (v) => set("controls", v)));
    wrap.append(node("small", supportsFirmware(0, 2, 19)
      ? "On the right of the double-width tile, like the rows in Home Assistant. Tapping the name works as configured below."
      : "The screen shows direct control from firmware 0.2.19; until then the tile stays as it was.", "field-hint"));
    body.append(wrap);
  }
  if (domain !== "screen") {
    const taps = [["auto", "Automatic"], ["detail", "Open control"], ["none", "View only"]];
    if (["light", "switch", "input_boolean", "fan", "media_player", "climate"].includes(domain)) taps.push(["toggle", "On / off"]);
    body.append(field("On tap", segmented(taps, current("tap", "auto"), (v) => set("tap", v))));
  }
  if (["light", "fan", "cover", "number", "input_number", "media_player"].includes(domain))
    body.append(field("Small slider on the tile", segmented([["none", "No"], ["slider", "Yes, control directly"]], current("inline", "none"), (v) => set("inline", v))));
  if (domain === "sensor")
    body.append(field("History", segmented([[1, "1 hour"], [6, "6 hours"], [24, "24 hours"]], current("history_hours", 24), (v) => set("history_hours", Number(v)))));
  const palette = node("div", undefined, "sheet-field");
  palette.append(node("span", "Pastel background"));
  const swatches = node("div", undefined, "palette-swatches");
  for (const [key, choice] of Object.entries(inventory.backgrounds || {})) {
    const button = node("button", undefined, "palette-choice");
    button.type = "button";
    button.setAttribute("aria-label", `Background: ${choice.label}`);
    button.setAttribute("aria-pressed", String((tile.options?.background || "auto") === key));
    const sample = node("span", undefined, "palette-sample");
    if (choice.color) sample.style.backgroundColor = choice.color;
    else sample.classList.add(key === "none" ? "palette-none" : "palette-auto");
    button.append(sample, node("span", choice.label));
    button.onclick = () => {
      tile.options = { ...tile.options, background: key };
      for (const other of swatches.children) other.setAttribute("aria-pressed", String(other === button));
      markDirty();
      renderPreview();
    };
    swatches.append(button);
  }
  palette.append(swatches);
  body.append(palette);
  const foot = node("div", undefined, "sheet-foot");
  const remove = node("button", "Remove", "quiet danger");
  remove.type = "button";
  remove.onclick = () => removeTile(sheetIndex);
  foot.append(remove);
  if (domain !== "screen") {
    const inspectButton = node("button", "Inspect", "quiet");
    inspectButton.type = "button";
    inspectButton.onclick = () => { const entity = tile.entity; closeTileSheet(); inspect(entity); };
    foot.append(inspectButton);
  }
  const done = node("button", "Done");
  done.type = "button";
  done.onclick = closeTileSheet;
  foot.append(done);
  sheet.append(head, body, foot);
}
// ---- Top bar ----
// The name on the left; on the right up to six items: the time, an analog clock, the date, or an
// entity's state or last change. The add-on formats entity text (POST header-preview) exactly as the
// screen gets it; the screen and this mockup tick clocks and "5 min ago" themselves.
const MONTHS = ["Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"];
const WEEKDAYS = ["Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"];
const BUILTIN_ICONS = { clock: "clock-outline", analog: "clock-outline", date: "calendar" };
let topbarPreviews = new Map(), topbarTimer = 0, topbarSheetIndex = null, topbarOverflow = new Set(), topbarAdded = null;
const itemKey = (item) => JSON.stringify(item);
// Without a stored top bar the screen shows what it always did: the clock of show_clock.
const topbarItems = () => layout.header?.items ?? ((layout.settings?.show_clock ?? true) ? [{ type: "clock" }] : []);
const topbarMax = () => inventory.header?.max_items || 6;
function setTopbarItems(items) {
  layout.header = { items };
  markDirty();
  renderTopbar();
  renderBars();
  loadTopbarPreview();
}
// A caption over a group of buttons; unlike a label it never forwards a click to the first one.
function group(title, content) {
  const wrap = node("div", undefined, "sheet-field");
  wrap.append(node("span", title), content);
  return wrap;
}
// Only the bars of the mockup pages, so tiles, focus and a running drag stay untouched.
function renderBars() {
  const bars = document.querySelectorAll("#layout-preview .preview-bar-wrap");
  if (!bars.length) return renderPreview();
  for (const bar of bars) bar.replaceWith(topbarBar());
  fitTopbars();
}
// Entity text as the screen will show it, for the items not previewed yet.
function loadTopbarPreview(delay = 150) {
  clearTimeout(topbarTimer);
  topbarTimer = setTimeout(async () => {
    const items = topbarItems();
    if (!items.some((item) => item.type === "entity")) return;
    try {
      const data = await (await api("header-preview", { method: "POST", body: JSON.stringify({ header: { items } }) })).json();
      items.forEach((item, i) => topbarPreviews.set(itemKey(item), data.items[i]));
      if (!layout) return;
      renderTopbar();
      renderBars();
      if (topbarSheetIndex !== null && $("#topbar-sheet").open) renderTopbarLive();
    } catch {
      // Keep the last preview; the next edit or refresh tries again.
    }
  }, delay);
}
function clockText(now = new Date()) {
  let hours = now.getHours();
  // The screen formats with %I:%M when the 24-hour clock is off.
  if (settingValues().clock_24h === false) hours = hours % 12 || 12;
  return `${String(hours).padStart(2, "0")}:${String(now.getMinutes()).padStart(2, "0")}`;
}
const dateText = (now = new Date()) => `${WEEKDAYS[now.getDay()]} ${now.getDate()} ${MONTHS[now.getMonth()]}`;
// Same wording and thresholds as header_bar::ago_text() in the firmware.
function agoText(then, now = Math.floor(Date.now() / 1000)) {
  const seconds = now - then, span = Math.abs(seconds), per = (unit) => Math.floor(span / unit);
  if (seconds < 0) {
    if (span < 3600) return `In ${Math.max(1, per(60))} min`;
    if (span < 86400) return per(3600) === 1 ? "In 1 hour" : `In ${per(3600)} hours`;
    if (span < 172800) return "Tomorrow";
    return `In ${per(86400)} days`;
  }
  if (span < 60) return "Just now";
  if (span < 3600) return `${per(60)} min ago`;
  if (span < 86400) return per(3600) === 1 ? "1 hour ago" : `${per(3600)} hours ago`;
  if (span < 172800) return "Yesterday";
  if (span < 604800) return `${per(86400)} days ago`;
  if (span < 2592000) return per(604800) === 1 ? "1 week ago" : `${per(604800)} weeks ago`;
  if (span < 31536000) return per(2592000) === 1 ? "1 month ago" : `${per(2592000)} months ago`;
  return per(31536000) === 1 ? "1 year ago" : `${per(31536000)} years ago`;
}
function topbarLabel(item) {
  if (item.type === "entity") return entityName(item.entity);
  return inventory.header?.builtin.find((b) => b.type === item.type)?.label || item.type;
}
// What the item shows right now: { icon, text, color, shown }. Entities wait for the add-on's preview.
function topbarView(item) {
  if (item.type === "clock") return { text: clockText(), shown: true };
  if (item.type === "date") return { text: dateText(), shown: true };
  if (item.type === "analog") return { analog: true, shown: true };
  const p = topbarPreviews.get(itemKey(item));
  if (!p) return { icon: item.icon === "none" ? null : iconNamed(item.icon)?.cp || automaticIcon(item.entity), text: "…", shown: true, loading: true };
  return { icon: p.i || null, text: p.k === "ago" ? agoText(p.e) : p.t, color: p.c ? `#${p.c}` : null, shown: p.shown };
}
// ---- The bar as the screen draws it ----
// One rule set with header_bar.h in the firmware, in screen pixels of the board: every value (the
// time too) in the same 400 Roboto on the name's baseline; icons and the dial centred on the
// height of the digits; the gaps measured between what you see (glyph ink), not between boxes, so
// an icon with side bearings sits exactly as close to its value as one without.
const BAR_METRICS = {
  guition: { width: 448, top: 36, name: 27, text: 21, icon: 26 },
  cyd: { width: 298, top: 24, name: 18, text: 14, icon: 18 },
};
// Screens without the Guition type sensor are CYDs, as the rotation setting assumes too.
const barMetrics = () => BAR_METRICS[inventory.screens.find((s) => s.id === selected)?.board === "guition" ? "guition" : "cyd"];
const measure = document.createElement("canvas").getContext("2d");
const inkCache = new Map();
// Ink box of a string relative to its origin on the baseline: left/right, and top (negative, up)/bottom.
function inkOf(text, font) {
  const key = `${font}|${text}`;
  if (!inkCache.has(key)) {
    measure.font = font;
    const m = measure.measureText(text);
    inkCache.set(key, { left: -m.actualBoundingBoxLeft, right: m.actualBoundingBoxRight, top: -m.actualBoundingBoxAscent, bottom: m.actualBoundingBoxDescent, advance: m.width });
  }
  return inkCache.get(key);
}
const barFonts = (m) => ({ name: `500 ${m.name}px "Bar Roboto"`, text: `400 ${m.text}px "Bar Roboto"`, icon: `${m.icon}px "Tile Icons"` });
// The fonts load on first use; measurements before that are wrong, so draw again once they are in.
Promise.all([document.fonts.load('500 27px "Bar Roboto"', "Studio 0"), document.fonts.load('400 21px "Bar Roboto"', "Away 0"), document.fonts.load('26px "Tile Icons"', String.fromCodePoint(0xf0150))])
  .then(() => { inkCache.clear(); if (layout) { renderTopbar(); renderBars(); } })
  .catch(() => {});
// Same integer arithmetic as header_bar::gaps() in the firmware, from the digit height in pixels.
function barGaps(cap) {
  return { icon: Math.max(2, Math.floor((cap * 4 + 5) / 10)), item: Math.max(6, Math.floor((cap * 125 + 50) / 100)), name: Math.max(8, Math.floor((cap * 16 + 5) / 10)) };
}
// The parts per item with their ink widths, the placement, and which items fall off.
function barLayout(items = topbarItems(), metrics = barMetrics(), nameText = $("#title").value || "Home") {
  const fonts = barFonts(metrics);
  // The firmware reads the digit height as a whole number of pixels (the glyph box of "0").
  const zero = inkOf("0", fonts.text), cap = Math.round(zero.bottom - zero.top), gaps = barGaps(cap);
  const dialInk = inkOf(String.fromCodePoint(0xf0150), fonts.icon), dial = Math.round(dialInk.bottom - dialInk.top);
  const parts = items.map((item, index) => {
    const view = topbarView(item);
    const part = { index, item, view, shown: view.shown, width: 0 };
    if (view.analog) { part.dial = dial; part.width = dial; return part; }
    if (view.icon) { part.icon = { glyph: glyph(view.icon), ink: inkOf(glyph(view.icon), fonts.icon) }; part.width += part.icon.ink.right - part.icon.ink.left; }
    if (view.text) {
      part.text = { value: view.text, ink: inkOf(view.text, fonts.text) };
      part.width += (part.icon ? gaps.icon : 0) + part.text.ink.right - part.text.ink.left;
    }
    return part;
  });
  const shown = parts.filter((p) => p.shown);
  const natural = inkOf(nameText, fonts.name).advance;
  const minName = Math.min(natural, Math.floor((metrics.width * 35) / 100));
  const total = (list) => list.reduce((sum, p) => sum + p.width, 0) + Math.max(0, list.length - 1) * gaps.item;
  let first = 0;
  while (first < shown.length && total(shown.slice(first)) + gaps.name + minName > metrics.width) first++;
  const placed = shown.slice(first), dropped = new Set(shown.slice(0, first).map((p) => p.index));
  let x = metrics.width - total(placed);
  for (const p of placed) { p.x = x; x += p.width + gaps.item; }
  const nameRoom = placed.length ? placed[0].x - gaps.name : metrics.width;
  return { metrics, fonts, cap, zero, gaps, parts, placed, dropped, nameText, natural, nameRoom };
}
// LVGL's LV_LABEL_LONG_DOT: the longest start that fits with "..." after it.
function dotted(text, font, room) {
  if (inkOf(text, font).advance <= room) return text;
  const chars = [...text];
  while (chars.length && inkOf(chars.join("") + "...", font).advance > room) chars.pop();
  return chars.join("") + "...";
}
// `font` goes through CSSOM: the add-on's CSP blocks style attributes.
const svgNode = (tag, attrs = {}) => {
  const el = document.createElementNS("http://www.w3.org/2000/svg", tag);
  for (const [k, v] of Object.entries(attrs)) {
    if (k === "font") el.style.font = v;
    else el.setAttribute(k, v);
  }
  return el;
};
// Draws placed parts into an SVG group; `baseline` is the text baseline in screen pixels.
function drawParts(group, lay, parts, baseline) {
  const capMiddle = baseline + (lay.zero.top + lay.zero.bottom) / 2;
  for (const p of parts) {
    const color = p.view.color || "#46525e";
    if (p.dial) {
      const d = p.dial, stroke = Math.max(1, Math.round(d / 10)), hand = Math.max(1, Math.round(d * 0.075));
      const cx = p.x + d / 2, now = new Date();
      group.append(svgNode("circle", { cx, cy: capMiddle, r: d / 2 - stroke / 2, fill: "none", stroke: color, "stroke-width": stroke }));
      for (const [angle, length] of [[(now.getHours() % 12 + now.getMinutes() / 60) * 30, d * 0.24], [now.getMinutes() * 6, d * 0.34]]) {
        const rad = (angle * Math.PI) / 180;
        group.append(svgNode("line", { x1: cx, y1: capMiddle, x2: cx + length * Math.sin(rad), y2: capMiddle - length * Math.cos(rad), stroke: color, "stroke-width": hand, "stroke-linecap": "round" }));
      }
      continue;
    }
    let x = p.x;
    if (p.icon) {
      const ink = p.icon.ink, icon = svgNode("text", { x: x - ink.left, y: capMiddle - (ink.top + ink.bottom) / 2, fill: color, font: lay.fonts.icon });
      icon.textContent = p.icon.glyph;
      group.append(icon);
      x += ink.right - ink.left + (p.text ? lay.gaps.icon : 0);
    }
    if (p.text) {
      const text = svgNode("text", { x: x - p.text.ink.left, y: baseline, fill: "#46525e", font: lay.fonts.text });
      text.textContent = p.text.value;
      group.append(text);
    }
  }
}
// The bar at the top of a mockup page, scaled with the page: name left, items right.
function topbarBar() {
  const lay = barLayout(), m = lay.metrics;
  const height = m.top + Math.round(m.name * 0.45);
  const svg = svgNode("svg", { viewBox: `0 0 ${m.width} ${height}`, class: "preview-bar", role: "img" });
  const name = svgNode("text", { x: 0, y: m.top, fill: "#1b1b1b", font: lay.fonts.name });
  name.textContent = dotted(lay.nameText, lay.fonts.name, Math.min(lay.natural, lay.nameRoom));
  svg.append(name);
  const group = svgNode("g");
  drawParts(group, lay, lay.placed, m.top);
  svg.append(group);
  svg.setAttribute("aria-label", `Top bar: ${lay.nameText}`);
  const wrap = node("div", undefined, "preview-bar-wrap");
  wrap.title = "Edit top bar";
  wrap.append(svg);
  wrap.onclick = () => { $("#topbar").scrollIntoView({ behavior: "smooth", block: "center" }); $("#topbar").classList.add("flash"); setTimeout(() => $("#topbar").classList.remove("flash"), 900); };
  return wrap;
}
// Which items the screen leaves out for lack of room; the chips mark them.
function fitTopbars() {
  const dropped = barLayout().dropped;
  const before = [...topbarOverflow].join();
  topbarOverflow = dropped;
  if ([...dropped].join() !== before) renderTopbar();
}
// One item on its own, drawn exactly as in the bar (the sheet's "how it looks on the screen").
function itemSample(item) {
  const lay = barLayout([item]), m = lay.metrics, part = lay.parts[0];
  part.x = 0;
  const top = Math.round(m.text * 1.05), height = Math.round(m.text * 1.4);
  const svg = svgNode("svg", { viewBox: `-2 0 ${Math.max(1, part.width) + 4} ${height}`, class: "topbar-sample", role: "img" });
  svg.style.width = `${(Math.max(1, part.width) + 4) * (26 / m.icon)}px`;
  const group = svgNode("g");
  drawParts(group, lay, [part], top);
  svg.append(group);
  return svg;
}
function renderTopbar() {
  if (!layout) return;
  const items = topbarItems(), chips = $("#topbar-chips");
  chips.replaceChildren();
  items.forEach((item, index) => chips.append(topbarChip(item, index)));
  const add = node("button", "＋ Add", "topbar-add");
  add.type = "button";
  add.disabled = items.length >= topbarMax();
  add.title = add.disabled ? `Maximum ${topbarMax()} items` : "Add the time, date, analog clock, or an entity";
  add.onclick = () => openTopbarSheet(-1);
  chips.append(add);
  $("#topbar-count").textContent = `${items.length} / ${topbarMax()}`;
  const needed = inventory.header?.min_firmware || "0.2.32";
  const [major, minor, patch] = needed.split(".").map(Number);
  const hint = $("#topbar-hint");
  hint.textContent = supportsFirmware(major, minor, patch)
    ? topbarOverflow.size ? "Not everything fits next to the name: the screen drops the dashed items. Remove one or choose a shorter name." : "Drag to reorder; tap an item to configure it."
    : `Sensors, the date, and the analog clock appear from firmware ${needed}; until that update, this screen shows the name and, if the time is in the bar, the clock.`;
  hint.classList.toggle("warn", topbarOverflow.size > 0 && supportsFirmware(major, minor, patch));
}
function topbarChip(item, index) {
  const view = topbarView(item);
  const chip = node("div", undefined, "topbar-chip");
  chip.setAttribute("role", "listitem");
  chip.tabIndex = 0;
  chip.dataset.index = index;
  chip.classList.toggle("is-hidden", !view.shown);
  chip.classList.toggle("is-overflow", topbarOverflow.has(index));
  chip.classList.toggle("just-added", topbarAdded?.key === itemKey(item) && Date.now() - topbarAdded.time < 1200);
  chip.classList.toggle("dragging-chip", chipDrag.active && chipDrag.index === index);
  const icon = node("span", "", "mdi chip-icon");
  const cp = view.analog || item.type !== "entity" ? iconNamed(BUILTIN_ICONS[item.type])?.cp : view.icon;
  if (cp) icon.textContent = glyph(cp);
  if (view.color) icon.style.color = view.color;
  const texts = node("span", undefined, "chip-text");
  const detail = !view.shown ? "Hidden: not active right now" : topbarOverflow.has(index) ? "Doesn't fit next to the name" : view.analog ? "Dial" : view.text;
  texts.append(node("strong", topbarLabel(item)), node("small", detail));
  const remove = node("button", "✕", "chip-remove");
  remove.type = "button";
  remove.setAttribute("aria-label", `Remove ${topbarLabel(item)} from the top bar`);
  remove.onclick = (e) => { e.stopPropagation(); removeTopbarItem(index); };
  chip.append(icon, texts, remove);
  chip.setAttribute("aria-label", `${topbarLabel(item)}, slot ${index + 1}. Enter: configure, arrows: move`);
  chip.onclick = () => openTopbarSheet(index);
  chip.onkeydown = (e) => {
    if (e.key === "Enter" || e.key === " ") { e.preventDefault(); openTopbarSheet(index); return; }
    const step = { ArrowLeft: -1, ArrowRight: 1 }[e.key];
    if (step && moveTopbarItem(index, index + step)) $(`#topbar-chips [data-index="${index + step}"]`)?.focus();
  };
  enableChipDrag(chip, index);
  return chip;
}
function moveTopbarItem(from, to) {
  const items = [...topbarItems()];
  if (to < 0 || to >= items.length || from === to) return false;
  items.splice(to, 0, ...items.splice(from, 1));
  setTopbarItems(items);
  return true;
}
function removeTopbarItem(index) {
  const items = [...topbarItems()];
  const [item] = items.splice(index, 1);
  if (!item) return;
  if ($("#topbar-sheet").open) $("#topbar-sheet").close();
  setTopbarItems(items);
  toast(`${topbarLabel(item)} removed from the top bar`, {
    label: "Undo",
    run: () => { const back = [...topbarItems()]; back.splice(Math.min(index, back.length), 0, item); setTopbarItems(back); },
  });
}
function addTopbarItem(item) {
  const items = topbarItems();
  if (items.length >= topbarMax()) return toast(`The top bar has room for ${topbarMax()} items.`);
  if (items.some((other) => itemKey(other) === itemKey(item))) return toast("This is already in the top bar.");
  // The new chip lights up briefly so the eye finds it.
  topbarAdded = { key: itemKey(item), time: Date.now() };
  setTopbarItems([...items, item]);
  $("#topbar-sheet").close();
}
// Pointer drag between chips, mouse and touch (touch after a short hold, so the page still scrolls).
// The order updates while dragging, the screen mockup follows, and a finished drag is not a click.
const chipDrag = { index: -1, start: null, timer: 0, active: false, pointerId: null, moved: false };
function enableChipDrag(chip, index) {
  chip.addEventListener("pointerdown", (e) => {
    if (e.button !== 0 || e.target.closest(".chip-remove")) return;
    if (e.pointerType !== "touch") e.preventDefault();
    Object.assign(chipDrag, { index, start: { x: e.clientX, y: e.clientY }, pointerId: e.pointerId, active: false, moved: false });
    clearTimeout(chipDrag.timer);
    if (e.pointerType === "touch") chipDrag.timer = setTimeout(() => beginChipDrag(), 260);
    document.addEventListener("pointermove", moveChipDrag);
    document.addEventListener("pointerup", endChipDrag);
    document.addEventListener("pointercancel", endChipDrag);
  });
}
function beginChipDrag() {
  chipDrag.active = true;
  document.body.classList.add("dragging");
  document.addEventListener("touchmove", blockChipScroll, { passive: false });
  $(`#topbar-chips [data-index="${chipDrag.index}"]`)?.classList.add("dragging-chip");
}
function blockChipScroll(e) { if (chipDrag.active) e.preventDefault(); }
function moveChipDrag(e) {
  if (e.pointerId !== chipDrag.pointerId || !chipDrag.start) return;
  if (!chipDrag.active) {
    const distance = Math.hypot(e.clientX - chipDrag.start.x, e.clientY - chipDrag.start.y);
    if (e.pointerType === "touch") { if (distance > 10) { clearTimeout(chipDrag.timer); chipDrag.start = null; } return; }
    if (distance < 6) return;
    beginChipDrag();
  }
  // The target is the chip under the pointer's row whose middle the pointer passed.
  const chips = [...document.querySelectorAll("#topbar-chips .topbar-chip")];
  let target = chipDrag.index;
  chips.forEach((chip, i) => {
    const r = chip.getBoundingClientRect();
    if (e.clientY >= r.top - 6 && e.clientY <= r.bottom + 6) {
      if (i < chipDrag.index && e.clientX < r.left + r.width / 2) target = Math.min(target, i);
      if (i > chipDrag.index && e.clientX > r.left + r.width / 2) target = Math.max(target, i);
    }
  });
  if (target !== chipDrag.index) {
    const items = [...topbarItems()];
    items.splice(target, 0, ...items.splice(chipDrag.index, 1));
    layout.header = { items };
    chipDrag.index = target;
    chipDrag.moved = true;
    renderTopbar();
    renderBars();
  }
}
function endChipDrag(e) {
  if (e.pointerId !== chipDrag.pointerId) return;
  clearTimeout(chipDrag.timer);
  document.removeEventListener("pointermove", moveChipDrag);
  document.removeEventListener("pointerup", endChipDrag);
  document.removeEventListener("pointercancel", endChipDrag);
  document.removeEventListener("touchmove", blockChipScroll);
  document.body.classList.remove("dragging");
  if (chipDrag.active) {
    drag.suppressUntil = Date.now() + 400;
    if (chipDrag.moved) markDirty();
    renderTopbar();
  }
  Object.assign(chipDrag, { index: -1, start: null, active: false, pointerId: null, moved: false });
}
// One sheet for adding (index -1) and for one item's settings.
function openTopbarSheet(index) {
  if (index !== topbarSheetIndex) iconPickerOpen = false;
  topbarSheetIndex = index;
  renderTopbarSheet();
  if (!$("#topbar-sheet").open) $("#topbar-sheet").showModal();
}
$("#topbar-sheet").addEventListener("close", () => { topbarSheetIndex = null; });
function sheetHead(badge, title, subtitle) {
  const head = node("div", undefined, "sheet-head"), titles = node("div");
  titles.append(node("strong", title), node("small", subtitle));
  const close = node("button", "✕", "quiet sheet-close");
  close.type = "button";
  close.setAttribute("aria-label", "Close");
  close.onclick = () => $("#topbar-sheet").close();
  head.append(badge, titles, close);
  return head;
}
function optionButton(cp, title, detail, onClick, disabled = false) {
  const b = node("button", undefined, "topbar-option");
  b.type = "button";
  b.disabled = disabled;
  const texts = node("span");
  texts.append(node("strong", title), node("small", detail));
  b.append(node("span", cp ? glyph(cp) : "", "mdi"), texts);
  b.onclick = onClick;
  return b;
}
function renderTopbarSheet() {
  const sheet = $("#topbar-sheet");
  sheet.replaceChildren();
  if (topbarSheetIndex === -1) return renderTopbarAdd(sheet);
  const item = topbarItems()[topbarSheetIndex];
  if (!item) return sheet.close();
  const badge = node("span", "", "domain-icon mdi");
  const body = node("div", undefined, "sheet-body");
  if (item.type === "entity") {
    const view = topbarView(item);
    badge.textContent = view.icon ? glyph(view.icon) : "";
    sheet.append(sheetHead(badge, entityName(item.entity), item.entity));
    const live = node("div", undefined, "topbar-live");
    live.id = "topbar-live";
    body.append(live);
    // Choices apply live; an icon pick keeps the open picker (search, scroll) and only refreshes the samples.
    const update = (patch) => {
      const items = [...topbarItems()];
      items[topbarSheetIndex] = { ...items[topbarSheetIndex], ...patch };
      setTopbarItems(items);
      renderTopbarLive();
      const next = topbarView(items[topbarSheetIndex]);
      badge.textContent = next.icon ? glyph(next.icon) : "";
    };
    const content = segmented(inventory.header.contents.map((c) => [c.key, c.label]), item.content, (v) => update({ content: v }));
    const contentField = group("What to show", content);
    contentField.append(node("small", "Last changed keeps counting on the screen itself: “Just now”, “5 min ago”, “Yesterday”.", "field-hint"));
    body.append(contentField);
    body.append(iconPicker({
      selected: item.icon,
      automatic: topbarPreviews.get(itemKey(item))?.auto_icon || automaticIcon(item.entity),
      autoLabel: "Automatic (like Home Assistant)",
      allowNone: true,
      onPick: (name) => update({ icon: name }),
    }));
    const shows = segmented(inventory.header.shows.map((s) => [s.key, s.label]), item.show, (v) => update({ show: v }));
    const showField = group("Show", shows);
    showField.append(node("small", "Only when active hides the item as long as it's off, closed, away, or 0. Handy for an open door, a running washing machine, or who's home.", "field-hint"));
    body.append(showField);
  } else {
    badge.textContent = glyph(iconNamed(BUILTIN_ICONS[item.type])?.cp || "");
    sheet.append(sheetHead(badge, topbarLabel(item), "From the screen itself, also works without Home Assistant"));
    const live = node("div", undefined, "topbar-live");
    live.id = "topbar-live";
    body.append(live);
    if (item.type !== "date") {
      // The screen's own clock setting: it applies at once, like the Clock row under Screen settings.
      const format = segmented([["24", "24 hour"], ["12", "12 hour"]], settingValues().clock_24h !== false ? "24" : "12", (v) => {
        setSetting("clock_24h", v === "24", 150);
      });
      const formatField = group("Format", format);
      formatField.append(node("small", "Applies to every clock on this screen, including the clock tiles.", "field-hint"));
      body.append(formatField);
    }
  }
  const foot = node("div", undefined, "sheet-foot");
  const remove = node("button", "Remove", "quiet danger");
  remove.type = "button";
  remove.onclick = () => removeTopbarItem(topbarSheetIndex);
  const left = node("button", "← Move left", "quiet"), right = node("button", "Move right →", "quiet");
  left.type = right.type = "button";
  left.disabled = topbarSheetIndex === 0;
  right.disabled = topbarSheetIndex >= topbarItems().length - 1;
  left.onclick = () => { if (moveTopbarItem(topbarSheetIndex, topbarSheetIndex - 1)) { topbarSheetIndex--; renderTopbarSheet(); } };
  right.onclick = () => { if (moveTopbarItem(topbarSheetIndex, topbarSheetIndex + 1)) { topbarSheetIndex++; renderTopbarSheet(); } };
  const done = node("button", "Done");
  done.type = "button";
  done.onclick = () => sheet.close();
  foot.append(remove, left, right, done);
  sheet.append(body, foot);
  renderTopbarLive();
}
// "How it looks": the item as the bar draws it, updated while choosing.
function renderTopbarLive() {
  const live = $("#topbar-live"), item = topbarItems()[topbarSheetIndex];
  if (!live || !item) return;
  const view = topbarView(item);
  const note = !view.shown ? "Hidden right now: not active" : topbarOverflow.has(topbarSheetIndex) ? "Doesn't currently fit next to the name" : "How it looks on the screen";
  live.replaceChildren(node("small", note), itemSample(item));
}
function renderTopbarAdd(sheet) {
  const badge = node("span", "＋", "domain-icon");
  sheet.append(sheetHead(badge, "Add to the top bar", `${topbarItems().length} of ${topbarMax()} slots used`));
  const body = node("div", undefined, "sheet-body");
  const taken = new Set(topbarItems().map(itemKey));
  const own = node("div", undefined, "topbar-options");
  const samples = { clock: clockText(), analog: "Small dial with the time", date: dateText() };
  for (const builtin of inventory.header?.builtin || []) {
    const item = { type: builtin.type };
    own.append(optionButton(iconNamed(BUILTIN_ICONS[builtin.type])?.cp, builtin.label, taken.has(itemKey(item)) ? "Already added" : samples[builtin.type], () => addTopbarItem(item), taken.has(itemKey(item))));
  }
  body.append(group("From the screen itself", own));
  const suggested = inventory.header?.suggestions?.[selected] || [];
  if (suggested.length) {
    const list = node("div", undefined, "topbar-options");
    for (const s of suggested) {
      const exists = taken.has(itemKey(s.item));
      list.append(optionButton(s.icon || automaticIcon(s.item.entity), s.label, exists ? "Already added" : [s.name, s.area].filter(Boolean).join(" · "), () => addTopbarItem(s.item), exists));
    }
    body.append(group("Suggestions from Home Assistant", list));
  }
  const search = node("input");
  search.type = "search";
  search.placeholder = "Search by name, room, or entity, e.g. temperature or door";
  const results = node("div", undefined, "topbar-results");
  const renderMatches = () => {
    const query = search.value.trim().toLocaleLowerCase();
    const matches = inventory.entities.filter((e) => `${e.name} ${e.id} ${e.area} ${e.device}`.toLocaleLowerCase().includes(query));
    results.replaceChildren(...matches.slice(0, 40).map((e) => {
      const item = { type: "entity", entity: e.id, content: "state", icon: "auto", show: "always" };
      return optionButton(automaticIcon(e.id), e.name, [e.area, e.id].filter(Boolean).join(" · "), () => addTopbarItem(item), taken.has(itemKey(item)));
    }));
    if (!matches.length) results.append(node("p", "No entities found.", "hint"));
    else if (matches.length > 40) results.append(node("p", `${matches.length} results. Keep typing to narrow it down.`, "hint"));
  };
  search.oninput = renderMatches;
  search.setAttribute("aria-label", "Search for an entity for the top bar");
  const searchField = group("An entity", search);
  searchField.append(results);
  body.append(searchField);
  renderMatches();
  const foot = node("div", undefined, "sheet-foot");
  const cancel = node("button", "Cancel", "quiet");
  cancel.type = "button";
  cancel.onclick = () => sheet.close();
  foot.append(cancel);
  sheet.append(body, foot);
}
// A click in the picker: the marked empty cell, else the first free cell.
function addTile(id) {
  if (layout.tiles.some((t) => t.entity === id) || layout.tiles.length >= tileLimit()) return;
  const tile = { entity: id, name: "", ...defaultOptions(id) };
  const slot = insertAt >= 0 ? insertAt : firstFree(occupied(liveEntries()), isWide(tile));
  insertAt = -1;
  selectedTile = id;
  if (slot >= 0) placeTile(tile, slot);
}
// Pointer-based drag & drop, mouse and touch, from the picker into the mockup and between
// cells. Touch starts after a short hold so the page still scrolls. While dragging, the
// mockup already shows where everything ends up; the drop confirms exactly that, and a
// drop off the grid changes nothing. A finished drag never doubles as a click.
const drag = { active: false, source: null, element: null, ghost: null, timer: 0, start: null, offset: null, pointerId: null, suppressUntil: 0, last: null, scroller: 0, moving: null, target: null, preview: null };
function enableDrag(element, source) {
  element.addEventListener("pointerdown", (e) => {
    if (e.button !== 0 || element.disabled || e.target.closest(".preview-remove")) return;
    // No text selection while the mouse drags; touch keeps its default so the page can scroll.
    if (e.pointerType !== "touch") e.preventDefault();
    Object.assign(drag, { source, element, start: { x: e.clientX, y: e.clientY }, pointerId: e.pointerId, active: false });
    // A fast flick may leave the card before its first move event: keep the pointer until the drag begins.
    try { element.setPointerCapture(e.pointerId); } catch {}
    clearTimeout(drag.timer);
    if (e.pointerType === "touch") drag.timer = setTimeout(() => beginDrag(e), 260);
  });
  element.addEventListener("pointermove", (e) => {
    if (drag.active || !drag.start || drag.element !== element) return;
    const distance = Math.hypot(e.clientX - drag.start.x, e.clientY - drag.start.y);
    if (e.pointerType === "touch") { if (distance > 10) { clearTimeout(drag.timer); drag.start = null; } return; }
    if (distance >= 6) beginDrag(e);
  });
  const cancel = () => { if (drag.element === element && !drag.active) { clearTimeout(drag.timer); drag.start = null; } };
  element.addEventListener("pointerup", cancel);
  element.addEventListener("pointercancel", cancel);
}
function beginDrag(e) {
  if (drag.active || !drag.start) return;
  drag.active = true;
  drag.moving = drag.source.kind === "tile" ? drag.source.tile : { entity: drag.source.id, name: "", ...defaultOptions(drag.source.id) };
  drag.target = null;
  drag.preview = null;
  getSelection()?.removeAllRanges();
  const rect = drag.element.getBoundingClientRect();
  const ghost = drag.element.cloneNode(true);
  ghost.classList.add("drag-ghost");
  ghost.style.width = `${rect.width}px`;
  drag.offset = { x: e.clientX - rect.left, y: e.clientY - rect.top };
  document.body.append(ghost);
  drag.ghost = ghost;
  document.body.classList.add("dragging");
  // The mockup re-renders while hovering, so the pointer is followed on the document, not the card.
  document.addEventListener("pointermove", moveDrag);
  document.addEventListener("pointerup", finishDrag);
  document.addEventListener("pointercancel", finishDrag);
  try { drag.element.releasePointerCapture(drag.pointerId); } catch {}
  try { document.documentElement.setPointerCapture(drag.pointerId); } catch {}
  document.addEventListener("touchmove", blockScroll, { passive: false });
  // Near the viewport edges the page scrolls along, so the mockup can be reached on small screens.
  drag.scroller = setInterval(() => {
    if (!drag.last) return;
    const step = drag.last.y < 70 ? -12 : drag.last.y > innerHeight - 70 ? 12 : 0;
    if (step) { window.scrollBy(0, step); setTarget(slotAt(drag.last.x, drag.last.y)); }
  }, 16);
  setTarget(-1);
  moveDrag(e);
}
function blockScroll(e) { if (drag.active) e.preventDefault(); }
function finishDrag(e) { if (e.pointerId === drag.pointerId) endDrag(e.type === "pointerup"); }
function moveDrag(e) {
  if (!drag.ghost || e.pointerId !== drag.pointerId) return;
  drag.last = { x: e.clientX, y: e.clientY };
  drag.ghost.style.transform = `translate(${e.clientX - drag.offset.x}px, ${e.clientY - drag.offset.y}px)`;
  setTarget(slotAt(e.clientX, e.clientY));
}
// The cell under the pointer: the nearest card or empty cell (the gaps between them count
// too); on a wide card the left or right half decides. -1 away from the mockup.
function slotAt(x, y) {
  let best = null, nearest = Infinity;
  for (const cell of document.querySelectorAll("#layout-preview [data-slot]")) {
    const r = cell.getBoundingClientRect();
    const distance = Math.hypot(Math.max(r.left - x, 0, x - r.right), Math.max(r.top - y, 0, y - r.bottom));
    if (distance < nearest) { nearest = distance; best = { cell, r }; }
  }
  if (!best || nearest > 16) return -1;
  let slot = Number(best.cell.dataset.slot);
  if (best.cell.classList.contains("wide") && x > (best.r.left + best.r.right) / 2) slot += 1;
  return slot;
}
function setTarget(slot) {
  if (drag.target === slot) return;
  drag.target = slot;
  const result = slot >= 0 ? arrange(layout.tiles, drag.moving, slot) : null;
  drag.preview = result;
  // Off the grid: a tile from the grid shows where it came from; a new one shows nowhere yet.
  renderPreview({ result: result || liveEntries(), moving: drag.moving });
}
function endDrag(drop) {
  const { preview } = drag;
  document.removeEventListener("pointermove", moveDrag);
  document.removeEventListener("pointerup", finishDrag);
  document.removeEventListener("pointercancel", finishDrag);
  document.removeEventListener("touchmove", blockScroll);
  try { document.documentElement.releasePointerCapture(drag.pointerId); } catch {}
  drag.ghost?.remove();
  Object.assign(drag, { ghost: null, active: false, suppressUntil: Date.now() + 400, last: null, start: null, moving: null, target: null, preview: null });
  clearInterval(drag.scroller);
  document.body.classList.remove("dragging");
  if (!(drop && preview && commit(preview))) renderPreview();
}
window.addEventListener("click", (e) => { if (Date.now() < drag.suppressUntil) { e.stopPropagation(); e.preventDefault(); } }, true);
function renderResults() {
  if (!layout) return;
  const query = $("#search").value.toLocaleLowerCase();
  const chosen = new Set(layout.tiles.map((t) => t.entity));
  const matches = [...(inventory.builtin || []), ...inventory.entities].filter(
    (e) =>
      (!filter || e.id.startsWith(filter + ".") ||
        ({switch:"input_boolean", number:"input_number", select:"input_select", weather:"sun"}[filter] === e.id.split(".")[0])) &&
      `${e.name} ${e.id} ${e.device} ${e.area}`
        .toLocaleLowerCase()
        .includes(query),
  );
  $("#results").replaceChildren();
  for (const entity of matches.slice(0, 80)) {
    const b = node("button", undefined, "result"),
      description = node("span");
    description.append(
      node("strong", entity.name),
      node("small", domains[entity.id.split(".")[0]]?.[0] || entity.id.split(".")[0], "domain-label"),
      node("small", [entity.area, entity.device].filter(Boolean).join(" · ")),
      node("small", entity.id),
    );
    b.append(
      domainBadge(entity.id),
      description,
      node("span", chosen.has(entity.id) ? "✓" : "+", "plus"),
    );
    b.disabled = chosen.has(entity.id) || layout.tiles.length >= tileLimit();
    b.onclick = () => addTile(entity.id);
    if (!b.disabled) enableDrag(b, { kind: "entity", id: entity.id });
    $("#results").append(b);
  }
  if (!matches.length)
    $("#results").append(
      node("p", "No entities found. Try a different name.", "hint"),
    );
  if (matches.length > 80)
    $("#results").append(
      node(
        "p",
        `${matches.length} results. Keep typing to narrow it down.`,
        "hint",
      ),
    );
}
async function refresh(full = true) {
  try {
    const data = await (await api(full ? "inventory" : "inventory?light=1")).json();
    // The live stream can open a screen before the first full inventory arrives (an add-on busy building
    // firmware answers it first); that screen was drawn without entity names and with an empty entity list.
    const firstCatalogue = full && !inventory.entities?.length;
    // A light poll carries only screens and update status; keep the catalogues we have.
    inventory = full ? data : { ...inventory, ...data };
    $("#connection").textContent = inventory.connected
      ? "● Home Assistant connected"
      : "Reconnecting to Home Assistant…";
    $("#connection").classList.toggle("online", inventory.connected);
    renderScreens();
    if (full) renderClaude();
    if (selected) { settleSettings(); renderSettings(); }
    if (firstCatalogue && selected && layout && !drag.active && !chipDrag.active) { renderTopbar(); renderTiles(); renderResults(); }
    if (!selected && inventory.screens.length) select(inventory.screens[0].id);
    if ($("#alerts-dialog").open) renderAlertScreens();
  } catch {
    $("#connection").textContent =
      "Management page unreachable · retrying…";
  }
}
$("#save").onclick = async () => {
  if (busy) return;
  busy = true;
  $("#save").disabled = true;
  try {
    layout.title = $("#title").value;
    // Screen settings apply on their own (flushSettings); the stored ones stay as they are.
    const { settings: _settings, ...tiles } = layout;
    await api(`screens/${encodeURIComponent(selected)}`, {
      method: "PUT",
      body: JSON.stringify(tiles),
    });
    dirty = false;
    $("#dirty").textContent = "Saved";
    $("#save-detail").textContent =
      "Sync follows automatically, even after reconnecting.";
    toast("Saved. Your screen is being updated.");
    await refresh();
  } catch (e) {
    toast(e.message);
  } finally {
    ((busy = false), (selectedTile = null));
    $("#save").disabled = false;
  }
};
$("#title").oninput = () => { markDirty(); renderPreview(); };
$("#add-page").onclick = addPage;
$("#search").oninput = renderResults;
$("#refresh").onclick = refresh;
for (const [value, label] of [
  ["", "All"],
  ["light", "Lights"],
  ["climate", "Climate"],
  ["switch", "Switches"],
  ["binary_sensor", "Status"],
  ["button", "Actions"],
  ["script", "Scripts"],
  ["fan", "Fans"],
  ["cover", "Covers"],
  ["scene", "Scenes"],
  ["vacuum", "Vacuum"],
  ["sensor", "Sensors"],
  ["media_player", "Media"],
  ["weather", "Weather"],
  ["number", "Values"],
  ["select", "Selects"],
  ["person", "People"],
  ["timer", "Timers"],
  ["screen", "Clock"],
]) {
  const b = node("button", label, value === "" ? "active" : "");
  if (value) b.prepend(domainBadge(value + "."));
  b.onclick = () => {
    filter = value;
    $("#filters")
      .querySelectorAll("button")
      .forEach((n) => n.classList.remove("active"));
    b.classList.add("active");
    renderResults();
  };
  $("#filters").append(b);
}
// ----- New screen: profile, Wi-Fi, and the first flash in one window -----
const installer = {
  poll: null, view: "setup", file: null, friendly: "", board: "cyd", target: "",
  apiKey: null, nodeEdited: false, ports: null, jobState: null,
};
// ESPHome's node-name rule: lowercase ASCII, digits and dashes, starting with a letter.
function slug(text) {
  const clean = text.toLowerCase().normalize("NFD").replace(/[̀-ͯ]/g, "")
    .replace(/[^a-z0-9]+/g, "-").replace(/^[^a-z]+/, "").slice(0, 30).replace(/-+$/, "");
  return clean || "screen";
}
function portLabel(port) {
  const id = port.replace(/^\/dev\/serial\/by-id\/usb-/, "").replace(/-if\d+(-port\d+)?$/, "").replace(/_/g, " ");
  return `USB · ${id === port ? port.replace(/^\/dev\//, "") : id}`;
}
function syncNode() {
  const form = $("#install-form");
  if (!installer.nodeEdited) form.elements.name.value = slug(form.elements.friendly_name.value);
  $("#node-preview").textContent = form.elements.name.value || "…";
}
function renderTargets(ports) {
  const select = $("#install-target");
  const key = ports.join("\n");
  if (installer.ports === key) return;
  installer.ports = key;
  const current = select.value;
  const later = node("option", "Later · save profile only");
  later.value = "";
  select.replaceChildren(...ports.map((p) => { const o = node("option", portLabel(p)); o.value = p; return o; }), later);
  select.value = ports.includes(current) ? current : ports[0] || "";
  renderTargetHint();
}
function renderTargetHint() {
  const select = $("#install-target");
  const ports = select.options.length - 1;
  $("#target-hint").textContent = !ports
    ? "No USB port found. Connect the screen with a data cable to the Home Assistant machine; the list refreshes on its own."
    : !select.value
      ? "The profile goes into the ESPHome folder. You can install later via Settings → Firmware & USB, or from ESPHome Device Builder."
      : ports > 1
        ? "More than one board connected: choose this screen's port."
        : "Once over USB; after that, everything is wireless.";
  $("#install-go").textContent = select.value ? "Install" : "Save profile";
}
function renderWifi(wifi) {
  const fields = $("#wifi-fields");
  const ask = wifi?.state === "new" || wifi?.state === "missing";
  fields.hidden = !ask;
  fields.disabled = !ask;
  if (ask) {
    const missing = wifi.missing || [];
    $("#wifi-ssid-label").hidden = !missing.includes("wifi_ssid");
    $("#wifi-password-label").hidden = !missing.includes("wifi_password");
    $("#wifi-status").textContent = wifi.state === "new"
      ? "Fill in once: ESP Screens saves this in ESPHome secrets.yaml, later screens use it automatically."
      : "Your ESPHome secrets.yaml is still missing Wi-Fi details. ESP Screens only fills in the missing lines.";
  }
  return wifi?.state === "ready"
    ? "Wi-Fi comes from your ESPHome secrets.yaml."
    : wifi?.state === "invalid"
      ? "secrets.yaml in the ESPHome folder isn't valid YAML. Fix the file first; it won't be overwritten."
      : "";
}
async function installerRefresh() {
  let data;
  try {
    data = await (await api("firmware")).json();
  } catch (e) {
    $("#install-note").textContent = e.message;
    return;
  }
  const job = data.job;
  const ours = job && installer.file && job.file === installer.file;
  if (ours) installer.jobState = job.state;
  if (installer.view === "setup") {
    if (ours && job.state === "running") return showProgress(job, data.logs);
    renderTargets(data.ports || []);
    const wifiNote = renderWifi(data.wifi);
    const busy = job?.state === "running";
    const flashing = !!$("#install-target").value;
    $("#install-note").textContent = busy
      ? `A build or installation is already running (${job.file}). Wait for it to finish.`
      : !data.available && flashing
        ? "The ESPHome CLI is missing from this installation; only saving the profile is possible."
        : wifiNote;
    $("#install-go").disabled = data.wifi?.state === "invalid" || (flashing && (busy || !data.available));
  } else if (installer.view === "progress" && ours) {
    renderProgress(job, data.logs);
  }
}
function showProgress(job, logs) {
  installer.view = "progress";
  installer.jobState = job.state;
  $("#install-setup").hidden = true;
  $("#install-progress").hidden = false;
  $("#install-log-wrap").hidden = false;
  renderProgress(job, logs);
}
function outcome(ok) {
  $("#progress-spin").hidden = true;
  $("#progress-mark").hidden = false;
  $("#progress-mark").textContent = ok ? "✓" : "✕";
  $("#progress-mark").className = `outcome ${ok ? "ok" : "bad"}`;
}
function renderProgress(job, logs) {
  const running = job.state === "running";
  const ok = job.state === "success";
  if (running) {
    $("#progress-spin").hidden = false;
    $("#progress-mark").hidden = true;
  } else outcome(ok);
  $("#install-title").textContent = running ? "One moment…" : ok ? "Done." : "That didn't work.";
  $("#progress-title").textContent = running
    ? job.stage === "upload" ? `Writing firmware to ${installer.friendly}…` : "Building firmware…"
    : ok ? `Firmware is on ${installer.friendly}` : "Install failed";
  $("#progress-detail").textContent = running
    ? job.stage === "upload"
      ? "Don't disconnect the USB cable yet."
      : "A first build takes a few minutes on a Raspberry Pi. You can close this window: the installation keeps running and you'll find it again under New screen."
    : ok
      ? `The screen boots up and connects to your Wi-Fi.${installer.board === "cyd" ? " The CYD first asks for a touch calibration: tap the crosshairs." : ""} Pair it with Home Assistant now:`
      : logs.filter((l) => /error/i.test(l)).pop() || logs.filter((l) => /failed/i.test(l)).pop() || "See the log below.";
  const pre = $("#install-log");
  const stick = pre.scrollTop + pre.clientHeight >= pre.scrollHeight - 8;
  pre.textContent = logs.join("\n");
  if (stick) pre.scrollTop = pre.scrollHeight;
  if (!running && !ok) $("#install-log-wrap").open = true;
  $("#install-result").hidden = !ok;
  if (ok) renderSteps();
  $("#install-retry").hidden = running || ok;
  $("#install-close").textContent = ok ? "Done" : "Close";
  $("#install-close").classList.toggle("quiet", !ok);
}
function showSaved() {
  installer.view = "done";
  $("#install-setup").hidden = true;
  $("#install-progress").hidden = false;
  outcome(true);
  $("#install-title").textContent = "Profile saved.";
  $("#progress-title").textContent = `${installer.file} is in the ESPHome folder`;
  $("#progress-detail").textContent =
    "You can install once the screen is connected to the Home Assistant machine: Settings → Firmware & USB → this profile → USB port → Build & install. Or open the profile in ESPHome Device Builder (same folder) and flash from your browser. Save the API key for pairing:";
  $("#install-result").hidden = false;
  renderSteps();
  $("#install-log-wrap").hidden = true;
  $("#install-retry").hidden = true;
  $("#install-close").textContent = "Done";
  $("#install-close").classList.remove("quiet");
}
function renderSteps() {
  $("#api-key").textContent = installer.apiKey || "";
  const steps = [
    ["Go to Home Assistant → Settings → Devices & services.", ` This happens outside ESP Screens. Home Assistant discovers ${installer.friendly} as an ESPHome device; click Add. Not discovered? Add ESPHome manually with the screen's IP address. `],
    ["Paste the API key", " above when Home Assistant asks for an encryption key."],
    ["Allow HA actions:", " ESPHome integration → Configure → “Allow the device to perform Home Assistant actions”. Without this, the screen sees everything but controls nothing."],
    ["Choose your tiles.", " Back in ESP Screens, the screen appears in the list on the left within about thirty seconds; until then it's shown there as “not yet in Home Assistant”."],
  ];
  $("#install-steps").replaceChildren(...steps.map(([b, t], i) => {
    const li = node("li");
    li.append(node("b", b), t);
    if (i === 0) {
      const go = node("button", "Open Devices & services", "mini");
      go.type = "button";
      go.onclick = openIntegrations;
      li.append(go);
    }
    return li;
  }));
}
function resetInstaller() {
  Object.assign(installer, { view: "setup", file: null, apiKey: null, nodeEdited: false, ports: null, jobState: null, target: "" });
  const form = $("#install-form");
  form.reset();
  $("#node-label").hidden = true;
  $("#install-setup").hidden = false;
  $("#install-progress").hidden = true;
  $("#install-result").hidden = true;
  $("#install-log-wrap").open = false;
  $("#install-log").textContent = "";
  $("#install-title").textContent = "Connect and install.";
  $("#install-status").textContent = "";
  $("#install-note").textContent = "";
  $("#install-target").replaceChildren();
  $("#target-hint").textContent = "";
  $("#install-close").classList.add("quiet");
  syncNode();
}
function openInstaller() {
  if (installer.view !== "progress") resetInstaller();
  $("#installer").showModal();
  installerRefresh();
  clearInterval(installer.poll);
  installer.poll = setInterval(installerRefresh, 3000);
}
$("#new-screen").onclick = $("#start").onclick = openInstaller;
$("#close-install").onclick = $("#install-close").onclick = () => $("#installer").close();
$("#installer").addEventListener("close", () => {
  clearInterval(installer.poll);
  // A finished job is shown once; the next open starts a fresh form.
  if (installer.view === "progress" && installer.jobState !== "running") installer.view = "done";
});
$("#install-target").onchange = () => { renderTargetHint(); installerRefresh(); };
$("#install-form").elements.friendly_name.oninput = syncNode;
$("#install-form").elements.name.oninput = () => { installer.nodeEdited = true; syncNode(); };
$("#edit-node").onclick = () => {
  installer.nodeEdited = true;
  $("#node-label").hidden = false;
  $("#install-form").elements.name.focus();
};
$("#install-form").onsubmit = async (e) => {
  e.preventDefault();
  const form = e.target;
  syncNode();
  if (!form.reportValidity()) return;
  const data = Object.fromEntries(new FormData(form));
  $("#install-go").disabled = true;
  $("#install-status").textContent = "";
  try {
    const result = await (await api("firmware/profiles", { method: "POST", body: JSON.stringify(data) })).json();
    Object.assign(installer, { file: result.file, apiKey: result.api_key, friendly: data.friendly_name.trim(), board: data.board, target: data.target });
    form.elements.wifi_password.value = "";
    if (result.job) showProgress(result.job, []);
    else showSaved();
  } catch (err) {
    $("#install-status").textContent = err.message;
  } finally {
    $("#install-go").disabled = false;
  }
};
$("#install-retry").onclick = async () => {
  try {
    const { ports } = await (await api("firmware")).json();
    // The board may have been replugged; a single visible port is unambiguous.
    if (!ports.includes(installer.target) && ports.length === 1) installer.target = ports[0];
    const job = await (await api("firmware/jobs", {
      method: "POST",
      body: JSON.stringify({ file: installer.file, action: "install", target: installer.target }),
    })).json();
    showProgress(job, []);
  } catch (err) {
    toast(err.message);
  }
};
$("#copy-key").onclick = () => copyText(installer.apiKey || "", $("#api-key"));
// A change still waiting for its short pause goes out when the page closes.
window.addEventListener("pagehide", () => flushSettings(true));
window.addEventListener("beforeunload", (e) => {
  if (dirty) {
    e.preventDefault();
    e.returnValue = "";
  }
});
refresh();
// Poll only while the tab is visible; a hidden tab would otherwise keep the add-on busy.
// Live updates arrive over server-sent events; polling is the fallback while the stream is down,
// plus a full catalogue refresh every 5 minutes.
let pollTimer, lastFull = Date.now(), live = false, stream;
function applyLive(data) {
  inventory = { ...inventory, ...data };
  $("#connection").textContent = inventory.connected ? "● Home Assistant connected" : "Reconnecting to Home Assistant…";
  $("#connection").classList.toggle("online", inventory.connected);
  renderScreens();
  if (selected) { settleSettings(); renderSettings(); }
  if (!selected && inventory.screens.length) select(inventory.screens[0].id);
}
function listen() {
  if (stream || typeof EventSource === "undefined") return;
  stream = new EventSource("api/events");
  stream.onopen = () => { live = true; poll(); };
  stream.onmessage = (e) => { if (!document.hidden) applyLive(JSON.parse(e.data)); };
  stream.onerror = () => { live = false; poll(); };
}
function poll() {
  clearTimeout(pollTimer);
  const wait = live ? 60000 : inventory.updates?.busy ? 3000 : 10000;
  pollTimer = setTimeout(async () => {
    if (!document.hidden) {
      const full = Date.now() - lastFull >= 300000;
      if (full) lastFull = Date.now();
      if (full || !live) await refresh(full);
    }
    poll();
  }, wait);
}
listen();
poll();
// The mockup's clocks tick and entity values in the top bar follow Home Assistant while the page is open.
setInterval(() => {
  if (!layout || document.hidden || drag.active || chipDrag.active) return;
  loadTopbarPreview(0);
  renderTopbar();
  renderBars();
}, 30000);
document.addEventListener("visibilitychange", async () => {
  if (document.hidden) return;
  lastFull = Date.now();
  await refresh();
  poll();
});

// Shared firmware workspace; always select a concrete profile and upload target.
let firmwarePoll;
async function firmwareRefresh(initial = false) {
  try {
    const data = await (await api("firmware")).json();
    if (initial) {
      $("#firmware-file").replaceChildren(
        ...data.profiles.map((p) => {
          const o = node("option", p.file);
          o.value = p.file;
          return o;
        }),
      );
      $("#firmware-port").replaceChildren(
        ...[["ota", "Wi-Fi / OTA"], ...data.ports.map((p) => [p, p])].map(
          ([value, text]) => {
            const o = node("option", text);
            o.value = value;
            return o;
          },
        ),
      );
    }
    const running = data.job?.state === "running";
    for (const id of ["validate", "build", "install"])
      $("#firmware-" + id).disabled =
        running || !data.available || !data.profiles.length;
    $("#firmware-status").textContent = !data.available
      ? "The ESPHome CLI is missing. Update the app to 0.2.0."
      : data.job
        ? `${data.job.file} · ${data.job.action} · ${data.job.state}`
        : "Choose the intended profile and a USB port or IP address.";
    $("#firmware-log").textContent = data.logs.join("\n");
  } catch (e) {
    toast(e.message);
  }
}
$("#open-firmware").onclick = () => {
  $("#firmware-dialog").showModal();
  firmwareRefresh(true);
  clearInterval(firmwarePoll);
  firmwarePoll = setInterval(() => firmwareRefresh(), 3000);
};
$("#close-firmware").onclick = () => {
  $("#firmware-dialog").close();
  clearInterval(firmwarePoll);
};
$("#firmware-dialog").addEventListener("close", () =>
  clearInterval(firmwarePoll),
);
$("#firmware-port").onchange = () =>
  ($("#firmware-host-label").hidden = $("#firmware-port").value !== "ota");
$("#firmware-file").onchange = () => {
  $("#firmware-host").placeholder = $("#firmware-file").value.replace(
    /\.yaml$/,
    ".local",
  );
};
for (const action of ["validate", "build", "install"])
  $("#firmware-" + action).onclick = async () => {
    try {
      await api("firmware/jobs", {
        method: "POST",
        body: JSON.stringify({
          file: $("#firmware-file").value,
          action:
            action === "validate"
              ? "validate"
              : action === "build"
                ? "build"
                : "install",
          target:
            $("#firmware-port").value === "ota"
              ? $("#firmware-host").value.trim()
              : $("#firmware-port").value,
        }),
      });
      await firmwareRefresh();
    } catch (e) {
      toast(e.message);
    }
  };
function openSection(id) {
  const section = $(id);
  if (section.tagName === "DETAILS") section.open = true;
  section.scrollIntoView({ behavior: "smooth", block: "start" });
}
async function inspect(entity) {
  if (!selected) return;
  openSection("#inspector-section");
  $("#inspection-summary").textContent = "Fetching current HA status...";
  try {
    const data = await (
      await api(`screens/${encodeURIComponent(selected)}/inspect`)
    ).json();
    const tiles = entity
      ? data.tiles.filter((t) => t.entity === entity)
      : data.tiles;
    $("#inspection-summary").replaceChildren();
    for (const tile of tiles) {
      const card = node("article", undefined, "inspection-tile");
      card.append(
        node("strong", tile.entity),
        node("p", `Status: ${tile.state}`),
      );
      const options =
        layout.tiles.find((t) => t.entity === tile.entity)?.options || {};
      card.append(
        node(
          "small",
          `Small slider: ${options.inline === "slider" ? "yes" : "no"} · Display: ${displayNames[options.display || "standard"] || options.display} · Width: ${options.size === "wide" ? "double" : "normal"} · Control: ${controlsLabel({ entity: tile.entity, options })} · Background: ${inventory.backgrounds?.[options.background || "auto"]?.label || "Default"}`,
        ),
      );
      $("#inspection-summary").append(card);
    }
    $("#inspection").textContent = JSON.stringify(
      entity ? tiles[0] : data,
      null,
      2,
    );
  } catch (e) {
    $("#inspection-summary").textContent = e.message;
    toast(e.message);
  }
}
$("#inspect").onclick = () => inspect();

// ----- Alerts: the cheatsheet for esphome.<node>_show_alert, built from the inventory -----
const versionAtLeast = (version, minimum) => {
  const parse = (v) => (/^(\d+)\.(\d+)\.(\d+)$/.exec(v || "") || []).slice(1).map(Number);
  const [a, b] = [parse(version), parse(minimum)];
  if (a.length !== 3 || b.length !== 3) return false;
  for (let i = 0; i < 3; i++) if (a[i] !== b[i]) return a[i] > b[i];
  return true;
};
const yamlString = (text) => `"${String(text).replace(/\\/g, "\\\\").replace(/"/g, '\\"')}"`;
function copyChip(text, what) {
  const b = node("button", "Copy", "mini");
  b.type = "button";
  b.onclick = () => copyText(text, undefined, what);
  return b;
}
function alertExampleYaml(action) {
  const fields = inventory.alerts?.fields || [];
  const lines = fields.map((f) => `  ${f.name}: ${f.type === "string" ? (/^[a-z][a-z0-9-]*$/.test(f.example) ? f.example : yamlString(f.example)) : f.example === true ? "true" : f.example === false ? "false" : f.example}`);
  return `action: ${action || "esphome.<device_name>_show_alert"}\ndata:\n${lines.join("\n")}`;
}
function alertWaitYaml(action) {
  return [
    `# Doorbell: show the alert and wait until someone presses the button.`,
    `actions:`,
    `  - action: ${action || "esphome.<device_name>_show_alert"}`,
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
    `        event_type: ${inventory.alerts?.event || "esphome.screen_alert"}`,
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
  ].join("\n");
}
// One event for every screen (app 0.2.45): an action for "Edit in YAML" of the Event action.
function alertAllYaml() {
  const fields = inventory.alerts?.fields || [];
  const lines = fields.map((f) => `  ${f.name}: ${f.type === "string" ? (/^[a-z][a-z0-9-]*$/.test(f.example) ? f.example : yamlString(f.example)) : String(f.example)}`);
  return `event: ${inventory.alerts?.broadcast?.show || "esp_screens_show_alert"}\nevent_data:\n${lines.join("\n")}`;
}
function renderAlertBroadcast() {
  const broadcast = inventory.alerts?.broadcast;
  if (broadcast) {
    $("#alerts-all-event").textContent = broadcast.show;
    $("#alerts-all-dismiss").textContent = broadcast.dismiss;
  }
  $("#alerts-all-example").textContent = alertAllYaml();
}
function renderAlertScreens() {
  const alerts = inventory.alerts, list = $("#alerts-screen-list");
  if (!alerts) return;
  $("#alerts-min-firmware").textContent = alerts.min_firmware;
  list.replaceChildren();
  if (!inventory.screens.length) {
    list.append(node("p", "No screen paired yet. Install one via New screen; the action appears once Home Assistant sees the screen.", "hint"));
  }
  for (const screen of inventory.screens) {
    const row = node("div", undefined, "alert-screen");
    const head = node("div", undefined, "alert-screen-head");
    head.append(node("strong", screen.name));
    const ready = versionAtLeast(screen.firmware, alerts.min_firmware) && screen.alert_action;
    const badge = node("span", ready ? `● firmware ${screen.firmware}` : screen.alert_action ? `Update needed · firmware ${screen.firmware || "unknown"}` : "Device name unknown · update the screen", `badge ${ready ? "online" : "update"}`);
    head.append(badge);
    row.append(head);
    for (const [label, action] of [["Show alert", screen.alert_action], ["Dismiss alert", screen.dismiss_action]]) {
      const line = node("div", undefined, "copy-line");
      line.append(node("span", label, "copy-label"), node("code", action || "esphome.<device_name>_show_alert"));
      if (action) line.append(copyChip(action, "Action name"));
      row.append(line);
    }
    list.append(row);
  }
  const select = $("#alerts-example-screen"), current = select.value;
  select.replaceChildren();
  for (const screen of inventory.screens.filter((s) => s.alert_action)) {
    const option = node("option", screen.name);
    option.value = screen.alert_action;
    select.append(option);
  }
  if (!select.options.length) {
    const option = node("option", "a screen (not paired yet)");
    option.value = "";
    select.append(option);
  }
  if ([...select.options].some((o) => o.value === current)) select.value = current;
  renderAlertExamples();
}
function renderAlertExamples() {
  const action = $("#alerts-example-screen").value;
  $("#alerts-example").textContent = alertExampleYaml(action);
  $("#alerts-wait-example").textContent = alertWaitYaml(action);
}
function alertIconCard(icon, label) {
  const b = node("button", undefined, "alert-icon");
  b.type = "button";
  b.title = `Copy ${icon.name}`;
  b.append(node("span", glyph(icon.cp), "mdi"), node("code", icon.name));
  if (label) b.append(node("small", label));
  b.onclick = () => copyText(icon.name, undefined, "Icon name");
  return b;
}
function renderAlertIcons() {
  const alerts = inventory.alerts, icons = inventory.icons;
  if (!alerts || !icons) return;
  const suggested = $("#alerts-suggested");
  suggested.replaceChildren();
  for (const icon of alerts.suggested_icons) {
    const chip = node("button", undefined, "chip");
    chip.type = "button";
    chip.append(node("span", glyph(icon.cp), "mdi"), node("code", icon.name));
    chip.onclick = () => copyText(icon.name, undefined, "Icon name");
    suggested.append(chip);
  }
  const query = $("#alerts-icon-search").value.trim().toLowerCase();
  const groups = $("#alerts-icon-groups");
  groups.replaceChildren();
  const all = [...icons.groups, { label: "Also available: control and weather icons", icons: alerts.extra_icons }];
  let shown = 0;
  for (const group of all) {
    const matches = group.icons.filter((i) => !query || i.name.includes(query) || (i.label || "").toLowerCase().includes(query));
    if (!matches.length) continue;
    shown += matches.length;
    const block = node("div", undefined, "alert-icon-group");
    block.append(node("p", `${group.label} · ${matches.length}`, "alerts-subhead"));
    const grid = node("div", undefined, "alert-icon-grid");
    for (const icon of matches) grid.append(alertIconCard(icon, icon.label));
    block.append(grid);
    groups.append(block);
  }
  if (!shown) groups.append(node("p", `No icon for "${query}". Unknown names show the warning triangle (${alerts.fallback_icon}).`, "hint"));
}
function renderAlertColors() {
  const alerts = inventory.alerts, swatches = $("#alerts-swatches");
  if (!alerts) return;
  swatches.replaceChildren();
  const white = node("button", undefined, "swatch");
  white.type = "button";
  const blank = node("i");
  blank.style.background = "#FFFFFF";
  white.append(blank, node("span", undefined));
  white.lastChild.append(node("code", "empty"), node("small", "White (default)"));
  white.onclick = () => copyText("", undefined, "Empty color");
  swatches.append(white);
  for (const colour of alerts.colors) {
    const b = node("button", undefined, "swatch");
    b.type = "button";
    const dot = node("i");
    dot.style.background = colour.color;
    const text = node("span");
    text.append(node("code", colour.name), node("small", `${colour.label} · ${colour.color}`));
    b.append(dot, text);
    b.onclick = () => copyText(colour.name, undefined, "Color name");
    swatches.append(b);
  }
}
function renderAlertTables() {
  const alerts = inventory.alerts;
  if (!alerts) return;
  const types = { string: "text", int: "number", bool: "on / off" };
  const table = $("#alerts-field-table");
  table.replaceChildren();
  const head = node("tr");
  for (const title of ["Field", "Type", "What it does", "Example", "Limit"]) head.append(node("th", title));
  table.append(head);
  for (const field of alerts.fields) {
    const row = node("tr");
    const name = node("td");
    name.append(node("code", field.name), node("small", field.label));
    const example = node("td");
    example.append(node("code", typeof field.example === "string" ? field.example : String(field.example)));
    const limit = alerts.limits.cyd[field.name];
    const cells = [name, node("td", types[field.type] || field.type), node("td", field.help), example,
                   node("td", limit ? `CYD ${limit} · Guition ${alerts.limits.guition[field.name]} bytes` : field.type === "int" ? "0 to 86400 s" : "—")];
    cells.forEach((cell, index) => cell.dataset.label = ["Field", "Type", "What it does", "Example", "Limit"][index]);
    row.append(...cells);
    table.append(row);
  }
  const endings = $("#alerts-ending-table");
  endings.replaceChildren();
  const endHead = node("tr");
  for (const title of ["action", "When"]) endHead.append(node("th", title));
  endings.append(endHead);
  for (const ending of alerts.endings) {
    const row = node("tr"), cell = node("td"), when = node("td", ending.label);
    cell.append(node("code", ending.action));
    cell.dataset.label = "action";
    when.dataset.label = "When";
    row.append(cell, when);
    endings.append(row);
  }
  $("#alerts-event-name").textContent = alerts.event;
  const doorbell = alerts.suggested_icons.find((i) => i.name === "doorbell");
  if (doorbell) $("#alert-mock-icon").textContent = glyph(doorbell.cp);
  const orange = alerts.colors.find((c) => c.name === "orange");
  if (orange) $("#alert-mock").querySelector(".alert-mock-card").style.background = orange.color;
}
function renderAlerts() {
  if (!inventory.alerts) {
    toast("The cheatsheet is still loading; try again in a moment.");
    return false;
  }
  renderAlertTables();
  renderAlertScreens();
  renderAlertBroadcast();
  renderAlertIcons();
  renderAlertColors();
  return true;
}
$("#open-alerts").onclick = () => {
  if (renderAlerts()) $("#alerts-dialog").showModal();
};
$("#close-alerts").onclick = () => $("#alerts-dialog").close();
$("#alerts-example-screen").onchange = renderAlertExamples;
$("#alerts-example-copy").onclick = () => copyText($("#alerts-example").textContent, $("#alerts-example"), "YAML");
$("#alerts-wait-copy").onclick = () => copyText($("#alerts-wait-example").textContent, $("#alerts-wait-example"), "YAML");
$("#alerts-all-copy").onclick = () => copyText($("#alerts-all-example").textContent, $("#alerts-all-example"), "YAML");
$("#alerts-icon-search").oninput = renderAlertIcons;
for (const b of document.querySelectorAll("#alerts-dialog [data-jump]")) {
  b.onclick = () => document.getElementById(b.dataset.jump)?.scrollIntoView({ behavior: "smooth", block: "start" });
}
