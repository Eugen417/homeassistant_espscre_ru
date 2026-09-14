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
    let message = "Dit lukte niet. Vernieuw de pagina en probeer opnieuw.";
    try {
      message = (await response.json()).error || message;
    } catch {}
    throw new Error(message);
  }
  return response;
}
function markDirty() {
  dirty = true;
  $("#dirty").textContent = "Niet-opgeslagen wijzigingen";
  $("#save-detail").textContent = "Klik op opslaan om je scherm bij te werken.";
}
function select(id) {
  if (
    id !== selected &&
    dirty &&
    !confirm(
      "Je hebt niet-opgeslagen wijzigingen. Toch een ander scherm openen?",
    )
  )
    return;
  selected = id;
  selectedTile = null;
  if ($("#tile-sheet").open) $("#tile-sheet").close();
  const screen = inventory.screens.find((s) => s.id === id);
  if (!screen) return;
  layout = structuredClone(screen.layout);
  dirty = false;
  $("#title").value = layout.title;
  $("#screen-name").textContent = screen.name;
  $("#editor").hidden = false;
  $("#empty").hidden = true;
  $("#dirty").textContent = "Alles opgeslagen";
  $("#save-detail").textContent = "Aanpassen kan zonder opnieuw flashen.";
  renderScreens();
  renderTiles();
  renderResults();
  renderSettings();
}
const settingDefinitions = [
  ["standby_enabled", "Automatisch standby", "check", true],
  ["standby_seconds", "Standby na", "minutes", 600, 1, 1440],
  ["brightness", "Helderheid normaal", "range", 100, 5, 100],
  ["standby_brightness", "Helderheid in standby", "range", 20, 0, 100],
  ["night_enabled", "Nachtstand gebruiken", "check", true],
  ["night_brightness", "Helderheid in nachtstand", "range", 10, 0, 100],
  ["night_start", "Nachtstand vanaf", "time", 1320],
  ["night_end", "Nachtstand tot", "time", 420],
  ["show_clock", "Klok tonen", "check", true],
  ["clock_24h", "24-uursklok (uit = 12 uur)", "check", true],
  ["home_on_standby", "Na standby terug naar pagina 1", "check", false],
  ["rotation", "Scherm draaien (met de klok mee)", "rotation", 0],
  ["swipe_pages", "Vegen tussen pagina’s (firmware 0.2.7+)", "check", false],
];
function renderSettings() {
  const values = {
    ...Object.fromEntries(
      settingDefinitions.map(([key, , , value]) => [key, value]),
    ),
    ...layout.settings,
  };
  const container = $("#settings-fields");
  container.replaceChildren();
  for (const [key, title, kind, , min, max] of settingDefinitions) {
    if(key === "rotation" && inventory.screens.find(s => s.id === selected)?.board !== "guition") continue;
    const label = node(
      "label",
      undefined,
      `setting ${kind === "check" ? "setting-check" : ""}`,
    );
    const caption = node("span", title),
      input = node(kind === "rotation" ? "select" : "input"),
      output = node("output");
    input.id = `setting-${key}`;
    if(kind !== "rotation") input.type =
      kind === "check" ? "checkbox" : kind === "minutes" ? "number" : kind;
    input.setAttribute("aria-label", title);
    input.required = kind === "minutes" || kind === "time";
    if(kind === "rotation") {
      for(const angle of [0,90,180,270]) { const option=node("option", `${angle}°`); option.value=angle; input.append(option); }
      input.value=values[key];
    }
    else if (kind === "check") input.checked = values[key];
    else if (kind === "time")
      input.value = `${String(Math.floor(values[key] / 60)).padStart(2, "0")}:${String(values[key] % 60).padStart(2, "0")}`;
    else {
      input.min = min;
      input.max = max;
      input.step = 1;
      input.value = kind === "minutes" ? values[key] / 60 : values[key];
    }
    const updateOutput = () =>
      (output.textContent =
        kind === "range"
          ? `${input.value}%`
          : kind === "minutes"
            ? "minuten"
            : "");
    updateOutput();
    input.oninput = () => {
      if (!input.checkValidity() || input.value === "") return;
      let value = kind === "check" ? input.checked : Number(input.value);
      if (kind === "time") {
        const [h, m] = input.value.split(":").map(Number);
        value = h * 60 + m;
      }
      if (kind === "minutes") value *= 60;
      layout.settings = { ...values, ...layout.settings, [key]: value };
      // A lower normal brightness also lowers any higher standby levels.
      if (key === "brightness")
        for (const dim of ["standby_brightness", "night_brightness"]) {
          layout.settings[dim] = Math.min(layout.settings[dim], value);
          const other = $(`#setting-${dim}`);
          other.value = layout.settings[dim];
          other.parentElement.querySelector("output").textContent =
            `${other.value}%`;
        }
      updateOutput();
      markDirty();
    };
    label.append(caption, input, output);
    container.append(label);
  }
  renderSettingsSupport();
}
function renderSettingsSupport() {
  const version =
    inventory.screens.find((s) => s.id === selected)?.firmware || "";
  const parts = /^([0-9]+)\.([0-9]+)\.([0-9]+)$/.exec(version);
  const supported =
    parts &&
    (Number(parts[1]) > 0 ||
      Number(parts[2]) > 1 ||
      (Number(parts[2]) === 1 && Number(parts[3]) >= 2));
  $("#settings-support").textContent = supported
    ? "Opslaan stuurt je instellingen direct naar dit scherm. Ze blijven ook na een herstart bewaard."
    : "Eenmalig firmware 0.1.2 of nieuwer installeren via ESPHome. Je kunt de instellingen alvast bewaren; oudere firmware gebruikt ze nog niet.";
}
const updating = new Set();
const PHASES = {
  install: "Bouwen en installeren…",
  verify: "Wachten tot het scherm terug is…",
  settle: "Controleren of het stabiel blijft…",
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
      node("small", PHASES[u.phase] || "Update starten…"),
    );
  } else if (u.state === "queued") {
    row.append(node("small", "In de wachtrij voor de update"));
  } else if (u.available && screen.online) {
    row.append(node("span", `Update ${u.target}`, "badge update"));
    const button = node("button", "Bijwerken", "mini");
    button.type = "button";
    if (u.host && u.profile) {
      button.onclick = (e) => {
        e.stopPropagation();
        startUpdate(screen);
      };
    } else if (!u.profile) {
      button.disabled = true;
      button.title = "Geen ESPHome-profiel met deze apparaatnaam gevonden.";
    } else {
      button.onclick = (e) => {
        e.stopPropagation();
        const form = node("form", undefined, "screen-host");
        const input = node("input");
        input.placeholder = "IP-adres, bijv. 192.168.1.50";
        input.required = true;
        input.pattern = "[A-Za-z0-9][A-Za-z0-9.\\-]*";
        const go = node("button", "Start", "mini");
        const cancel = node("button", "✕", "quiet");
        cancel.type = "button";
        cancel.setAttribute("aria-label", "Annuleren");
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
          node("small", "Eenmalig het IP-adres; nieuwe firmware meldt het zelf."),
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
  // so send the top window to Apparaten & diensten (same origin); elsewhere open a tab.
  const path = "/config/integrations/dashboard";
  try {
    window.top.location.assign(path);
  } catch {
    window.open(path, "_blank");
  }
}
async function copyText(text, element) {
  try {
    if (!navigator.clipboard || !window.isSecureContext) throw new Error();
    await navigator.clipboard.writeText(text);
    toast("API-sleutel gekopieerd.");
  } catch {
    if (element) {
      const range = document.createRange();
      range.selectNodeContents(element);
      const selection = window.getSelection();
      selection.removeAllRanges();
      selection.addRange(range);
    }
    toast(document.execCommand("copy") ? "API-sleutel gekopieerd." : "De sleutel is geselecteerd. Kopieer met Ctrl+C of Command+C.");
  }
}
// Profiles that Home Assistant does not list yet: the freshly flashed screen is not lost, it
// still has to be added under Apparaten & diensten, outside this page.
function renderPending() {
  const box = $("#pending");
  box.replaceChildren();
  for (const p of inventory.pending || []) {
    const card = node("div", undefined, "pending");
    card.append(
      node("strong", p.friendly),
      node("small", p.installed
        ? "Geïnstalleerd, maar nog niet in Home Assistant. Dat doe je buiten ESP Screens: voeg het ontdekte ESPHome-apparaat toe onder Instellingen → Apparaten & diensten, plak daar de API-sleutel en zet bij Configureren “Allow the device to perform Home Assistant actions” aan."
        : `Nog niet in Home Assistant. Al geflasht? Voeg het ESPHome-apparaat toe onder Instellingen → Apparaten & diensten en sta daarna bij Configureren de Home Assistant-acties toe. Nog niet geflasht? Firmware & USB → ${p.file}.`),
    );
    const actions = node("div", undefined, "pending-actions");
    const go = node("button", "Open Apparaten & diensten", "mini");
    go.type = "button";
    go.onclick = openIntegrations;
    actions.append(go);
    if (p.api_key) {
      const copy = node("button", "Kopieer API-sleutel", "mini quiet");
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
    b.append(
      node("strong", screen.name),
      node(
        "small",
        `${screen.online ? "● Online" : "○ Offline"}${screen.area ? " · " + screen.area : ""} · firmware ${screen.firmware || "onbekend"}`,
      ),
    );
    b.onclick = () => select(screen.id);
    item.append(b);
    const update = renderUpdate(screen);
    if (update) item.append(update);
    $("#screens").append(item);
  }
  const screen = inventory.screens.find((s) => s.id === selected);
  if (screen) {
    $("#delivery").textContent = screen.online
      ? `${screen.delivery} · ${screen.status}`
      : "Offline · wijzigingen worden bewaard";
    $("#delivery").classList.toggle("online", screen.online);
  }
  renderUpdates();
}
function renderUpdates() {
  const u = inventory.updates;
  $("#updates").hidden = !u || !inventory.screens.length;
  if (!u) return;
  const outdated = inventory.screens.filter((s) => s.update?.available).length;
  $("#updates-hint").textContent = u.busy
    ? `Bezig met bijwerken naar firmware ${u.target}…`
    : outdated
      ? `Firmware ${u.target} is beschikbaar voor ${outdated} scherm${outdated === 1 ? "" : "en"}.`
      : `Alle schermen hebben firmware ${u.target}.`;
  $("#update-all").hidden = !u.pending || !!u.busy || u.pending < 2;
  $("#update-all").textContent = `Alle ${u.pending} schermen bijwerken`;
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
$("#auto-update").onchange = async () => {
  try {
    await api("updates", {
      method: "PUT",
      body: JSON.stringify({ auto: $("#auto-update").checked }),
    });
    toast(
      $("#auto-update").checked
        ? "Schermen worden voortaan 's nachts bijgewerkt."
        : "Automatisch bijwerken staat uit.",
    );
  } catch (e) {
    $("#auto-update").checked = !$("#auto-update").checked;
    toast(e.message);
  }
};
function move(from, to) {
  if (to < 0 || to >= layout.tiles.length) return;
  const [tile] = layout.tiles.splice(from, 1);
  layout.tiles.splice(to, 0, tile);
  markDirty();
  renderTiles();
}
const domains = {
  light: ["Licht", "☀", "#ad7600", "#fff3d3"],
  climate: ["Klimaat", "❄", "#c86620", "#ffebdc"],
  vacuum: ["Stofzuiger", "◉", "#008577", "#def3ed"],
  fan: ["Ventilator", "✣", "#008aab", "#def5fa"],
  cover: ["Zonwering", "▤", "#8053af", "#eee5f8"],
  media_player: ["Media", "▶", "#007cad", "#def2fc"],
  sensor: ["Sensor", "⌁", "#3476b1", "#e5effa"],
  binary_sensor: ["Status", "◈", "#ad7600", "#fff3d3"],
  switch: ["Schakelaar", "⏻", "#ad7600", "#fff3d3"],
  input_boolean: ["Schakelaar", "⏻", "#ad7600", "#fff3d3"],
  scene: ["Scène", "✦", "#8053af", "#eee5f8"],
  script: ["Script", "▷", "#8053af", "#eee5f8"],
  weather: ["Weer", "☁", "#007cad", "#def2fc"],
  number: ["Waarde", "±", "#008577", "#def3ed"],
  input_number: ["Waarde", "±", "#008577", "#def3ed"],
  select: ["Keuze", "≡", "#5862af", "#eaecfa"],
  input_select: ["Keuze", "≡", "#5862af", "#eaecfa"],
  button: ["Actie", "↗", "#5862af", "#eaecfa"],
  screen: ["Klok", "◷", "#25282c", "#e9ecf1"],
  sun: ["Zon", "☼", "#c86620", "#ffebdc"],
  timer: ["Kookwekker", "⏱", "#008577", "#def3ed"],
  person: ["Persoon", "☺", "#2f7d32", "#e1f2e2"],
};
const displayNames = { standard: "standaard", watch: "grote waarde", forecast: "weersvoorspelling", graph: "grafiek", digital: "digitale klok", analog: "analoge klok", sunpath: "zonnebaan" };
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
  if (!key) return "geen";
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
  else if (key === "run") box.append(node("b", { scene: "Activeren", script: "Uitvoeren" }[domain] || "Indrukken", "preview-run"));
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
// Same packing as the firmware: wide tiles start in the left column and take a whole row.
function packTiles(tiles) {
  let position = 0;
  const placement = tiles.map((tile) => {
    const wide = tile.options?.size === "wide";
    if (wide && position % 2 === 1) position++;
    const slot = { page: Math.floor(position / 6), slot: position % 6, wide };
    position += wide ? 2 : 1;
    return slot;
  });
  return { placement, pages: Math.max(1, Math.ceil(position / 6)) };
}
function supportsFirmware(major, minor, patch) {
  const version = inventory.screens.find((s) => s.id === selected)?.firmware || "";
  const match = /^(\d+)\.(\d+)\.(\d+)$/.exec(version);
  if (!match) return false;
  const [a, b, c] = match.slice(1).map(Number);
  return a > major || (a === major && (b > minor || (b === minor && c >= patch)));
}
function domainBadge(id) {
  const [title, symbol, color, background] = domains[id.split(".")[0]] || ["Entiteit", "◇", "#637184", "#edf0f4"];
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
function renderPreview() {
  const root = $("#layout-preview");
  root.replaceChildren();
  const { placement, pages } = packTiles(layout.tiles);
  for (let page = 0; page < pages; page++) {
    const frame = node("section", undefined, "screen-preview");
    frame.append(node("small", `Pagina ${page + 1} · ${$("#title").value || "Thuis"}`, "preview-heading"));
    const grid = node("div", undefined, "preview-grid");
    let filled = 0;
    layout.tiles.forEach((tile, index) => {
      const place = placement[index];
      if (place.page !== page) return;
      // Keep the grid honest: an empty right column before a wide tile is a real gap on the screen.
      for (; filled < place.slot; filled++) grid.append(node("span", "", "preview-gap"));
      const card = node("div", undefined, "preview-tile");
      card.tabIndex = 0;
      card.setAttribute("role", "button");
      const name = tile.name || entityName(tile.entity);
      const background=inventory.backgrounds?.[tile.options?.background]?.color;
      if(background)card.style.backgroundColor=background;
      // "Geen": no card on the screen; the mockup keeps a dashed outline as drop target.
      if (tile.options?.background === "none") card.classList.add("bare");
      if (place.wide) card.classList.add("wide");
      card.append(tileBadge(tile), node("strong", name));
      card.dataset.index = index;
      enableDrag(card, { kind: "tile", index });
      card.classList.toggle("chosen", selectedTile === tile.entity);
      card.setAttribute("aria-label", `Tegel ${index + 1}: ${name}, instellen`);
      card.onclick = () => openTileSheet(index);
      card.onkeydown = (e) => { if (e.key === "Enter" || e.key === " ") { e.preventDefault(); openTileSheet(index); } };
      const remove = node("button", "✕", "preview-remove");
      remove.type = "button";
      remove.title = "Tegel verwijderen";
      remove.setAttribute("aria-label", `${name} verwijderen`);
      remove.onclick = (e) => { e.stopPropagation(); removeTile(index); };
      card.append(remove);
      if (tile.options?.inline === "slider") card.append(node("span", "", "preview-slider"));
      const controls = controlsPreview(tile);
      if (controls) card.append(controls);
      const display = tile.options?.display;
      if (display && display !== "standard") card.append(node("small", displayNames[display] || display));
      grid.append(card);
      filled += place.wide ? 2 : 1;
    });
    if (layout.tiles.length < tileLimit())
      for (; filled < 6; filled++) {
        const card = node("button", undefined, "preview-tile vacant");
        card.append(node("span", "+"), node("small", "Tegel toevoegen"));
        card.onclick = () => { $("#search").focus(); $("#search").scrollIntoView({behavior:"smooth", block:"center"}); };
        grid.append(card);
      }
    frame.append(grid);
    root.append(frame);
  }
}
function renderTiles() {
  renderPreview();
  $("#count").textContent = `${layout.tiles.length} / ${tileLimit()}${tileLimit()===10?" · update firmware voor 20":""}`;
  $("#no-tiles").hidden = layout.tiles.length > 0;
  if (sheetIndex >= 0) renderTileSheet();
}
function removeTile(index) {
  const [tile] = layout.tiles.splice(index, 1);
  if (!tile) return;
  if (sheetIndex >= 0) closeTileSheet();
  markDirty();
  renderTiles();
  renderResults();
  toast(`${tile.name || entityName(tile.entity)} verwijderd`, {
    label: "Ongedaan maken",
    run: () => { layout.tiles.splice(Math.min(index, layout.tiles.length), 0, tile); markDirty(); renderTiles(); renderResults(); },
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
  const wrap = node("div", undefined, "sheet-field");
  wrap.append(node("span", "Icoon"));
  const fromHA = Boolean(inventory.entities.find((e) => e.id === tile.entity)?.icon);
  const autoLabel = `Automatisch (${fromHA ? "uit Home Assistant" : "standaard"})`;
  const summary = node("button", undefined, "icon-current");
  summary.type = "button";
  summary.setAttribute("aria-expanded", String(iconPickerOpen));
  const current = node("span", undefined, "mdi"), text = node("span"), action = node("small", iconPickerOpen ? "Sluiten" : "Wijzigen");
  summary.append(current, text, action);
  const describe = () => {
    const chosen = iconNamed(tile.options?.icon);
    current.textContent = glyph(chosen?.cp || automaticIcon(tile.entity));
    text.textContent = chosen?.label || autoLabel;
  };
  describe();
  const panel = node("div", undefined, "icon-picker");
  panel.hidden = !iconPickerOpen;
  summary.onclick = () => {
    iconPickerOpen = panel.hidden;
    panel.hidden = !iconPickerOpen;
    summary.setAttribute("aria-expanded", String(iconPickerOpen));
    action.textContent = iconPickerOpen ? "Sluiten" : "Wijzigen";
  };
  const choice = (name, cp, label, cls = "") => {
    const b = node("button", undefined, `icon-choice ${cls}`);
    b.type = "button";
    b.dataset.icon = name;
    b.dataset.search = label.toLocaleLowerCase();
    b.title = label;
    b.setAttribute("aria-label", `Icoon: ${label}`);
    b.setAttribute("aria-pressed", String((tile.options?.icon || "auto") === name));
    b.append(node("span", glyph(cp), "mdi"));
    b.onclick = () => {
      tile.options = { ...tile.options, icon: name };
      for (const other of panel.querySelectorAll(".icon-choice")) other.setAttribute("aria-pressed", String(other === b));
      describe();
      markDirty();
      renderPreview();
      onChange();
    };
    return b;
  };
  const search = node("input");
  search.type = "search";
  search.placeholder = "Zoek, bijvoorbeeld lamp, muziek of deur";
  search.setAttribute("aria-label", "Zoek een icoon");
  const auto = choice("auto", automaticIcon(tile.entity), autoLabel, "icon-auto");
  auto.append(node("span", autoLabel));
  const list = node("div", undefined, "icon-list"), empty = node("p", "Geen icoon gevonden.", "hint");
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
  panel.append(search, auto, list, empty);
  wrap.append(summary, panel);
  if (!supportsFirmware(0, 2, 18)) wrap.append(node("small", "Het scherm toont een gekozen icoon vanaf firmware 0.2.18.", "icon-hint"));
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
  close.setAttribute("aria-label", "Sluiten");
  close.onclick = closeTileSheet;
  let badge = tileBadge(tile);
  head.append(badge, titles, close);
  const body = node("div", undefined, "sheet-body");
  const nameInput = node("input");
  nameInput.value = tile.name;
  nameInput.placeholder = name;
  nameInput.maxLength = 60;
  nameInput.oninput = () => { tile.name = nameInput.value; markDirty(); renderPreview(); };
  body.append(field("Naam op het scherm", nameInput));
  // The clock, forecast and sun path cards draw no tile icon.
  if (inventory.icons && domain !== "screen" && !["forecast", "sunpath"].includes(tile.options?.display))
    body.append(iconField(tile, () => { const next = tileBadge(tile); badge.replaceWith(next); badge = next; }));
  const displays = domain === "screen"
    ? [["digital", "Digitale klok"], ["analog", "Analoge klok"]]
    : [["standard", "Naam en status"], ["watch", "Grote waarde"]];
  if (domain === "weather") displays.push(["forecast", "Weersvoorspelling"]);
  if (domain === "sensor") displays.push(["graph", "Grafiek"]);
  if (domain === "sun") displays.push(["sunpath", "Zonnebaan"]);
  const current = (key, fallback) => tile.options?.[key] ?? fallback;
  const set = (key, value) => {
    tile.options = { ...tile.options, [key]: value };
    // Direct controls need the standard layout without a mini slider, and vice versa.
    if (key === "display" && value === "watch") { tile.options.inline = "none"; if (inventory.controls?.[domain]) tile.options.controls = "none"; }
    if (key === "display" && ["forecast", "sunpath"].includes(value)) tile.options.size = "wide";
    if (key === "inline" && value === "slider") { tile.options.display = "standard"; if (inventory.controls?.[domain]) tile.options.controls = "none"; }
    if (key === "controls" && value !== "none") { tile.options.display = "standard"; tile.options.inline = "none"; }
    markDirty();
    renderTiles();
  };
  body.append(field("Weergave", segmented(displays, current("display", domain === "screen" ? "digital" : "standard"), (v) => set("display", v))));
  body.append(field("Breedte", segmented([["single", "Normaal"], ["wide", "Dubbelbreed"]], current("size", "single"), (v) => set("size", v))));
  const catalogue = inventory.controls?.[domain];
  if (catalogue && current("size", "single") === "wide") {
    const wrap = field("Directe bediening op de tegel", segmented(catalogue.choices.map((c) => [c.key, c.label]), current("controls", catalogue.default), (v) => set("controls", v)));
    wrap.append(node("small", supportsFirmware(0, 2, 19)
      ? "Rechts op de dubbelbrede tegel, zoals de rijen in Home Assistant. Tikken op de naam werkt zoals hieronder ingesteld."
      : "Het scherm toont directe bediening vanaf firmware 0.2.19; tot die tijd blijft de tegel zoals hij was.", "field-hint"));
    body.append(wrap);
  }
  if (domain !== "screen") {
    const taps = [["auto", "Automatisch"], ["detail", "Bediening openen"], ["none", "Alleen bekijken"]];
    if (["light", "switch", "input_boolean", "fan", "media_player", "climate"].includes(domain)) taps.push(["toggle", "Aan / uit"]);
    body.append(field("Bij aantikken", segmented(taps, current("tap", "auto"), (v) => set("tap", v))));
  }
  if (["light", "fan", "cover", "number", "input_number", "media_player"].includes(domain))
    body.append(field("Kleine slider op de tegel", segmented([["none", "Nee"], ["slider", "Ja, direct bedienen"]], current("inline", "none"), (v) => set("inline", v))));
  if (domain === "sensor")
    body.append(field("Geschiedenis", segmented([[1, "1 uur"], [6, "6 uur"], [24, "24 uur"]], current("history_hours", 24), (v) => set("history_hours", Number(v)))));
  const palette = node("div", undefined, "sheet-field");
  palette.append(node("span", "Pastel achtergrond"));
  const swatches = node("div", undefined, "palette-swatches");
  for (const [key, choice] of Object.entries(inventory.backgrounds || {})) {
    const button = node("button", undefined, "palette-choice");
    button.type = "button";
    button.setAttribute("aria-label", `Achtergrond: ${choice.label}`);
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
  const remove = node("button", "Verwijderen", "quiet danger");
  remove.type = "button";
  remove.onclick = () => removeTile(sheetIndex);
  foot.append(remove);
  if (domain !== "screen") {
    const inspectButton = node("button", "Inspecteer", "quiet");
    inspectButton.type = "button";
    inspectButton.onclick = () => { const entity = tile.entity; closeTileSheet(); inspect(entity); };
    foot.append(inspectButton);
  }
  const done = node("button", "Klaar");
  done.type = "button";
  done.onclick = closeTileSheet;
  foot.append(done);
  sheet.append(head, body, foot);
}
function addTile(id, at) {
  if (layout.tiles.some((t) => t.entity === id) || layout.tiles.length >= tileLimit()) return;
  layout.tiles.splice(Math.min(at, layout.tiles.length), 0, { entity: id, name: "", ...defaultOptions(id) });
  selectedTile = id;
  markDirty();
  renderTiles();
  renderResults();
}
// Pointer-based drag & drop: works with mouse and touch, from the picker into
// the mockup and between tiles. Touch starts after a short hold so the page
// still scrolls; a finished drag never doubles as a click.
const drag = { active: false, source: null, element: null, ghost: null, target: null, timer: 0, start: null, offset: null, pointerId: null, suppressUntil: 0, last: null, scroller: 0 };
function enableDrag(element, source) {
  element.addEventListener("pointerdown", (e) => {
    if (e.button !== 0 || element.disabled || e.target.closest(".preview-remove")) return;
    Object.assign(drag, { source, element, start: { x: e.clientX, y: e.clientY }, pointerId: e.pointerId, active: false });
    clearTimeout(drag.timer);
    if (e.pointerType === "touch") drag.timer = setTimeout(() => beginDrag(e), 260);
  });
  element.addEventListener("pointermove", (e) => {
    if (!drag.start || drag.element !== element) return;
    const distance = Math.hypot(e.clientX - drag.start.x, e.clientY - drag.start.y);
    if (!drag.active) {
      if (e.pointerType === "touch") { if (distance > 10) { clearTimeout(drag.timer); drag.start = null; } return; }
      if (distance < 6) return;
      beginDrag(e);
    }
    moveDrag(e);
  });
  const finish = (e) => {
    if (drag.element !== element) return;
    clearTimeout(drag.timer);
    if (drag.active) endDrag(e.type === "pointerup");
    drag.start = null;
  };
  element.addEventListener("pointerup", finish);
  element.addEventListener("pointercancel", finish);
}
function beginDrag(e) {
  if (drag.active || !drag.start) return;
  drag.active = true;
  try { drag.element.setPointerCapture(drag.pointerId); } catch {}
  const rect = drag.element.getBoundingClientRect();
  const ghost = drag.element.cloneNode(true);
  ghost.classList.add("drag-ghost");
  ghost.style.width = `${rect.width}px`;
  drag.offset = { x: e.clientX - rect.left, y: e.clientY - rect.top };
  document.body.append(ghost);
  drag.ghost = ghost;
  document.body.classList.add("dragging");
  document.addEventListener("touchmove", blockScroll, { passive: false });
  // Near the viewport edges the page scrolls along, so the mockup can be reached on small screens.
  drag.scroller = setInterval(() => {
    if (!drag.last) return;
    const step = drag.last.y < 70 ? -12 : drag.last.y > innerHeight - 70 ? 12 : 0;
    if (step) { window.scrollBy(0, step); setTarget(document.elementFromPoint(drag.last.x, drag.last.y)?.closest(".preview-tile, .preview-gap, .preview-grid") || null); }
  }, 16);
  moveDrag(e);
}
function blockScroll(e) { if (drag.active) e.preventDefault(); }
function moveDrag(e) {
  if (!drag.ghost) return;
  drag.last = { x: e.clientX, y: e.clientY };
  drag.ghost.style.transform = `translate(${e.clientX - drag.offset.x}px, ${e.clientY - drag.offset.y}px)`;
  const under = document.elementFromPoint(e.clientX, e.clientY);
  setTarget(under?.closest(".preview-tile, .preview-gap, .preview-grid") || null);
}
function setTarget(target) {
  if (drag.target === target) return;
  drag.target?.classList.remove("drop-target");
  drag.target = target;
  drag.target?.classList.add("drop-target");
}
function endDrag(drop) {
  const { target, source } = drag;
  setTarget(null);
  drag.ghost?.remove();
  drag.ghost = null;
  drag.active = false;
  drag.suppressUntil = Date.now() + 400;
  clearInterval(drag.scroller);
  drag.last = null;
  document.body.classList.remove("dragging");
  document.removeEventListener("touchmove", blockScroll);
  try { drag.element.releasePointerCapture(drag.pointerId); } catch {}
  if (!drop || !target) return;
  const to = target.dataset.index !== undefined ? Number(target.dataset.index) : layout.tiles.length;
  if (source.kind === "tile") { if (to !== source.index) move(source.index, to); }
  else addTile(source.id, to);
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
    b.onclick = () => addTile(entity.id, layout.tiles.length);
    if (!b.disabled) enableDrag(b, { kind: "entity", id: entity.id });
    $("#results").append(b);
  }
  if (!matches.length)
    $("#results").append(
      node("p", "Geen entiteiten gevonden. Probeer een andere naam.", "hint"),
    );
  if (matches.length > 80)
    $("#results").append(
      node(
        "p",
        `${matches.length} resultaten. Typ verder om te verfijnen.`,
        "hint",
      ),
    );
}
async function refresh(full = true) {
  try {
    const data = await (await api(full ? "inventory" : "inventory?light=1")).json();
    // A light poll carries only screens and update status; keep the catalogues we have.
    inventory = full ? data : { ...inventory, ...data };
    $("#connection").textContent = inventory.connected
      ? "● Home Assistant verbonden"
      : "Verbinding met Home Assistant herstellen…";
    $("#connection").classList.toggle("online", inventory.connected);
    renderScreens();
    if (selected) renderSettingsSupport();
    if (!selected && inventory.screens.length) select(inventory.screens[0].id);
  } catch {
    $("#connection").textContent =
      "Beheerpagina niet bereikbaar · opnieuw proberen…";
  }
}
$("#save").onclick = async () => {
  if (busy) return;
  for (const input of document.querySelectorAll("#settings-fields input")) {
    if (!input.reportValidity()) return;
  }
  busy = true;
  $("#save").disabled = true;
  try {
    layout.title = $("#title").value;
    await api(`screens/${encodeURIComponent(selected)}`, {
      method: "PUT",
      body: JSON.stringify(layout),
    });
    dirty = false;
    $("#dirty").textContent = "Opgeslagen";
    $("#save-detail").textContent =
      "Synchronisatie volgt automatisch, ook na opnieuw verbinden.";
    toast("Opgeslagen. Je scherm wordt bijgewerkt.");
    await refresh();
  } catch (e) {
    toast(e.message);
  } finally {
    ((busy = false), (selectedTile = null));
    $("#save").disabled = false;
  }
};
$("#title").oninput = () => { markDirty(); renderPreview(); };
$("#search").oninput = renderResults;
$("#refresh").onclick = refresh;
for (const [value, label] of [
  ["", "Alles"],
  ["light", "Lampen"],
  ["climate", "Klimaat"],
  ["switch", "Schakelaars"],
  ["binary_sensor", "Status"],
  ["button", "Acties"],
  ["script", "Scripts"],
  ["fan", "Ventilatoren"],
  ["cover", "Zonwering"],
  ["scene", "Scènes"],
  ["vacuum", "Vacuum"],
  ["sensor", "Sensoren"],
  ["media_player", "Media"],
  ["weather", "Weer"],
  ["number", "Waarden"],
  ["select", "Keuzelijsten"],
  ["person", "Personen"],
  ["timer", "Kookwekkers"],
  ["screen", "Klok"],
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
// ----- Nieuw scherm: profiel, wifi en de eerste flash in één venster -----
const installer = {
  poll: null, view: "setup", file: null, friendly: "", board: "cyd", target: "",
  apiKey: null, nodeEdited: false, ports: null, jobState: null,
};
// ESPHome's node-name rule: lowercase ASCII, digits and dashes, starting with a letter.
function slug(text) {
  const clean = text.toLowerCase().normalize("NFD").replace(/[̀-ͯ]/g, "")
    .replace(/[^a-z0-9]+/g, "-").replace(/^[^a-z]+/, "").slice(0, 30).replace(/-+$/, "");
  return clean || "scherm";
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
  const later = node("option", "Later · alleen het profiel bewaren");
  later.value = "";
  select.replaceChildren(...ports.map((p) => { const o = node("option", portLabel(p)); o.value = p; return o; }), later);
  select.value = ports.includes(current) ? current : ports[0] || "";
  renderTargetHint();
}
function renderTargetHint() {
  const select = $("#install-target");
  const ports = select.options.length - 1;
  $("#target-hint").textContent = !ports
    ? "Geen USB-poort gevonden. Sluit het scherm met een datakabel aan op de Home Assistant-machine; de lijst ververst vanzelf."
    : !select.value
      ? "Het profiel komt in de ESPHome-map. Installeren kan later via Firmware & USB, of vanuit ESPHome Device Builder."
      : ports > 1
        ? "Meer dan één bord aangesloten: kies de poort van dit scherm."
        : "Eén keer via USB; daarna gaat alles draadloos.";
  $("#install-go").textContent = select.value ? "Installeren" : "Profiel bewaren";
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
      ? "Eén keer invullen: ESP Screens bewaart dit in ESPHome secrets.yaml, volgende schermen gebruiken het automatisch."
      : "Je ESPHome secrets.yaml mist nog wifi-gegevens. ESP Screens vult alleen de ontbrekende regels aan.";
  }
  return wifi?.state === "ready"
    ? "Wifi komt uit je ESPHome secrets.yaml."
    : wifi?.state === "invalid"
      ? "secrets.yaml in de ESPHome-map is geen geldige YAML. Herstel het bestand eerst; het wordt niet overschreven."
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
      ? `Er loopt al een build of installatie (${job.file}). Wacht tot die klaar is.`
      : !data.available && flashing
        ? "ESPHome CLI ontbreekt in deze installatie; alleen het profiel bewaren kan."
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
  $("#install-title").textContent = running ? "Even geduld…" : ok ? "Klaar." : "Dat lukte niet.";
  $("#progress-title").textContent = running
    ? job.stage === "upload" ? `Firmware naar ${installer.friendly} schrijven…` : "Firmware bouwen…"
    : ok ? `Firmware staat op ${installer.friendly}` : "Installeren mislukt";
  $("#progress-detail").textContent = running
    ? job.stage === "upload"
      ? "Haal de USB-kabel nog niet los."
      : "Een eerste build duurt op een Raspberry enkele minuten. Je mag dit venster sluiten: de installatie loopt door en je vindt hem terug onder Nieuw scherm."
    : ok
      ? `Het scherm start op en verbindt met je wifi.${installer.board === "cyd" ? " De CYD vraagt eerst om een touch-kalibratie: tik de kruisjes aan." : ""} Koppel het nu aan Home Assistant:`
      : logs.filter((l) => /error/i.test(l)).pop() || logs.filter((l) => /failed|mislukt|fout/i.test(l)).pop() || "Bekijk het log hieronder.";
  const pre = $("#install-log");
  const stick = pre.scrollTop + pre.clientHeight >= pre.scrollHeight - 8;
  pre.textContent = logs.join("\n");
  if (stick) pre.scrollTop = pre.scrollHeight;
  if (!running && !ok) $("#install-log-wrap").open = true;
  $("#install-result").hidden = !ok;
  if (ok) renderSteps();
  $("#install-retry").hidden = running || ok;
  $("#install-close").textContent = ok ? "Klaar" : "Sluiten";
  $("#install-close").classList.toggle("quiet", !ok);
}
function showSaved() {
  installer.view = "done";
  $("#install-setup").hidden = true;
  $("#install-progress").hidden = false;
  outcome(true);
  $("#install-title").textContent = "Profiel bewaard.";
  $("#progress-title").textContent = `${installer.file} staat in de ESPHome-map`;
  $("#progress-detail").textContent =
    "Installeren kan zodra het scherm aan de Home Assistant-machine hangt: Firmware & USB → dit profiel → USB-poort → Bouwen & installeren. Of open het profiel in ESPHome Device Builder (dezelfde map) en flash vanuit je browser. Bewaar de API-sleutel voor de koppeling:";
  $("#install-result").hidden = false;
  renderSteps();
  $("#install-log-wrap").hidden = true;
  $("#install-retry").hidden = true;
  $("#install-close").textContent = "Klaar";
  $("#install-close").classList.remove("quiet");
}
function renderSteps() {
  $("#api-key").textContent = installer.apiKey || "";
  const steps = [
    ["Ga naar Home Assistant → Instellingen → Apparaten & diensten.", ` Dit gebeurt buiten ESP Screens. Home Assistant ontdekt ${installer.friendly} als ESPHome-apparaat; klik op Toevoegen. Niet ontdekt? Voeg ESPHome handmatig toe met het IP-adres van het scherm. `],
    ["Plak de API-sleutel", " hierboven zodra Home Assistant om een encryptiesleutel vraagt."],
    ["Sta HA-acties toe:", " ESPHome-integratie → Configureren → “Allow the device to perform Home Assistant actions”. Zonder dit ziet het scherm alles, maar bedient het niets."],
    ["Kies je tegels.", " Terug in ESP Screens verschijnt het scherm binnen een halve minuut in de lijst links; tot die tijd staat het daar als “nog niet in Home Assistant”."],
  ];
  $("#install-steps").replaceChildren(...steps.map(([b, t], i) => {
    const li = node("li");
    li.append(node("b", b), t);
    if (i === 0) {
      const go = node("button", "Open Apparaten & diensten", "mini");
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
  $("#install-title").textContent = "Aansluiten en installeren.";
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
  $("#connection").textContent = inventory.connected ? "● Home Assistant verbonden" : "Verbinding met Home Assistant herstellen…";
  $("#connection").classList.toggle("online", inventory.connected);
  renderScreens();
  if (selected) renderSettingsSupport();
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
        ...[["ota", "Wifi / OTA"], ...data.ports.map((p) => [p, p])].map(
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
      ? "ESPHome CLI ontbreekt. Werk de app bij naar 0.2.0."
      : data.job
        ? `${data.job.file} · ${data.job.action} · ${data.job.state}`
        : "Kies het bedoelde profiel en een USB-poort of IP-adres.";
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
$("#nav-tiles").onclick = () => openSection("#tile-section");
$("#open-help").onclick = () => openSection("#help");
$("#nav-settings").onclick = () => openSection("#general-settings");
$("#nav-inspector").onclick = () => inspect();
async function inspect(entity) {
  if (!selected) return;
  openSection("#inspector-section");
  $("#inspection-summary").textContent = "Actuele HA-status ophalen...";
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
          `Kleine slider: ${options.inline === "slider" ? "ja" : "nee"} · Weergave: ${displayNames[options.display || "standard"] || options.display} · Breedte: ${options.size === "wide" ? "dubbel" : "normaal"} · Bediening: ${controlsLabel({ entity: tile.entity, options })} · Achtergrond: ${inventory.backgrounds?.[options.background || "auto"]?.label || "Standaard"}`,
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
