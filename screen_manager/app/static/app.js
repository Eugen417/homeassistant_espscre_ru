"use strict";
const $ = (s) => document.querySelector(s);
let inventory = { screens: [], entities: [] },
  selected = null,
  layout = null,
  dirty = false,
  filter = "",
  dragIndex = -1,
  busy = false,
  selectedTile = null;
const node = (tag, text, cls) => {
  const n = document.createElement(tag);
  if (text !== undefined) n.textContent = text;
  if (cls) n.className = cls;
  return n;
};
function toast(message) {
  $("#toast").textContent = message;
  $("#toast").hidden = false;
  clearTimeout(toast.timer);
  toast.timer = setTimeout(() => ($("#toast").hidden = true), 5000);
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
function renderScreens() {
  $("#screens").replaceChildren();
  for (const screen of inventory.screens) {
    const b = node(
      "button",
      undefined,
      `screen ${screen.id === selected ? "selected" : ""}`,
    );
    b.append(
      node("strong", screen.name),
      node(
        "small",
        `${screen.online ? "● Online" : "○ Offline"}${screen.area ? " · " + screen.area : ""} · firmware ${screen.firmware || "onbekend"}`,
      ),
    );
    b.onclick = () => select(screen.id);
    $("#screens").append(b);
  }
  const screen = inventory.screens.find((s) => s.id === selected);
  if (screen) {
    $("#delivery").textContent = screen.online
      ? `${screen.delivery} · ${screen.status}`
      : "Offline · wijzigingen worden bewaard";
    $("#delivery").classList.toggle("online", screen.online);
  }
}
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
};
function domainBadge(id) {
  const [title, symbol, color, background] = domains[id.split(".")[0]] || ["Entiteit", "◇", "#637184", "#edf0f4"];
  const badge = node("span", symbol, "domain-icon");
  badge.title = title;
  badge.setAttribute("aria-label", title);
  badge.style.color = color;
  badge.style.background = background;
  return badge;
}
function tileLimit() {
  const version=inventory.screens.find(s=>s.id===selected)?.firmware || "";
  const match=/^(\d+)\.(\d+)\.(\d+)$/.exec(version);
  if(!match)return 10;
  return Number(match[1])>0 || Number(match[2])>2 || Number(match[2])===2 && Number(match[3])>=7 ? 20 : 10;
}
function renderPreview() {
  const root = $("#layout-preview");
  root.replaceChildren();
  const pages = Math.max(1, Math.ceil(layout.tiles.length / 6));
  for (let page = 0; page < pages; page++) {
    const frame = node("section", undefined, "screen-preview");
    frame.append(node("small", `Pagina ${page + 1} · ${$("#title").value || "Thuis"}`, "preview-heading"));
    const grid = node("div", undefined, "preview-grid");
    for (let slot = page * 6; slot < Math.min(page * 6 + 6, tileLimit()); slot++) {
      const tile = layout.tiles[slot];
      const card = node("button", undefined, "preview-tile");
      if (tile) {
        const name = tile.name || inventory.entities.find(e => e.id === tile.entity)?.name || tile.entity;
        card.append(domainBadge(tile.entity), node("strong", name));
        card.draggable = true;
        card.ondragstart = (e) => { dragIndex = slot; e.dataTransfer.setData("text/plain", String(slot)); };
        card.ondragend = () => { dragIndex = -1; };
        card.ondragover = (e) => { if (dragIndex >= 0) e.preventDefault(); };
        card.ondrop = (e) => { e.preventDefault(); if (dragIndex >= 0) move(dragIndex, slot); };
        card.classList.toggle("chosen", selectedTile === tile.entity);
        card.setAttribute("aria-label", `Tegel ${slot + 1}: ${name}, instellen`);
        card.onclick = () => {
          selectedTile = tile.entity;
          renderTiles();
          document.querySelectorAll("#tiles > .tile")[slot]?.scrollIntoView({behavior: "smooth", block: "center"});
        };
        if (tile.options?.inline === "slider") card.append(node("span", "", "preview-slider"));
        if (tile.options?.display === "watch") card.append(node("small", "Grote waarde"));
      } else {
        card.classList.add("vacant");
        card.append(node("span", "+"), node("small", "Tegel toevoegen"));
        card.onclick = () => { $("#search").focus(); $("#search").scrollIntoView({behavior:"smooth", block:"center"}); };
      }
      grid.append(card);
    }
    frame.append(grid);
    root.append(frame);
  }
}
function renderTiles() {
  renderPreview();
  $("#tiles").replaceChildren();
  $("#count").textContent = `${layout.tiles.length} / ${tileLimit()}${tileLimit()===10?" · update firmware voor 20":""}`;
  $("#no-tiles").hidden = layout.tiles.length > 0;
  layout.tiles.forEach((tile, i) => {
    if (i === 6) $("#tiles").append(node("li", "Pagina 2", "page-break"));
    const li = node("li", undefined, "tile");
    li.classList.toggle("selected-tile", selectedTile === tile.entity);
    li.draggable = true;
    li.ondragstart = (e) => {
      if (e.target.closest("input, select, button")) {
        e.preventDefault();
        return;
      }
      dragIndex = i;
      e.dataTransfer.setData("text/plain", String(i));
      li.classList.add("dragging");
    };
    li.ondragend = () => {
      dragIndex = -1;
      li.classList.remove("dragging");
    };
    li.ondragover = (e) => {
      if (dragIndex >= 0) e.preventDefault();
    };
    li.ondrop = (e) => {
      e.preventDefault();
      if (dragIndex >= 0) move(dragIndex, i);
    };
    const content = node("div"),
      input = node("input");
    input.value = tile.name;
    input.placeholder =
      inventory.entities.find((e) => e.id === tile.entity)?.name || tile.entity;
    input.maxLength = 60;
    input.setAttribute("aria-label", `Naam voor tegel ${i + 1}`);
    input.oninput = () => {
      tile.name = input.value;
      markDirty();
      renderPreview();
    };
    content.append(input, node("small", tile.entity));
    const actions = node("div", undefined, "tile-actions");
    for (const [label, title, action, disabled] of [
      ["↑", "Omhoog", () => move(i, i - 1), i === 0],
      ["↓", "Omlaag", () => move(i, i + 1), i === layout.tiles.length - 1],
      [
        "✕",
        "Verwijderen",
        () => {
          layout.tiles.splice(i, 1);
          markDirty();
          renderTiles();
          renderResults();
        },
        false,
      ],
    ]) {
      const b = node("button", label);
      b.title = title;
      b.setAttribute("aria-label", title);
      b.disabled = disabled;
      b.onclick = action;
      actions.append(b);
    }
    const options = node("details", undefined, "tile-options");
    options.append(node("summary", "Bediening & weergave instellen"));
    options.open = selectedTile === tile.entity;
    options.ontoggle = () => {
      li.classList.toggle("selected-tile", options.open);
      if (options.open) {
        selectedTile = tile.entity;
        renderPreview();
        document.querySelectorAll(".tile-options").forEach((other) => {
          if (other !== options) other.open = false;
        });
      }
    };
    li.onclick = (e) => {
      if (!e.target.closest("input, select, button, summary, .tile-options"))
        options.open = !options.open;
    };
    const controls = {};
    const domain = tile.entity.split(".")[0];
    const fields = [
      [
        "tap",
        "Bij aantikken",
        [
          ["auto", "Automatisch"],
          ["detail", "Bediening openen"],
          ["none", "Alleen bekijken"],
        ],
      ],
      [
        "display",
        "Weergave",
        [
          ["standard", "Naam en status"],
          ["watch", "Grote waarde"],
        ],
      ],
    ];
    if (
      ["light", "switch", "input_boolean", "fan", "media_player"].includes(
        domain,
      )
    )
      fields[0][2].push(["toggle", "Aan / uit"]);
    if (
      [
        "light",
        "fan",
        "cover",
        "number",
        "input_number",
        "media_player",
      ].includes(domain)
    )
      fields.push([
        "inline",
        "Kleine slider op deze tegel?",
        [
          ["none", "Nee"],
          ["slider", "Ja, direct bedienen"],
        ],
      ]);
    if (domain === "sensor")
      fields.push([
        "history_hours",
        "Geschiedenis",
        [
          [1, "1 uur"],
          [6, "6 uur"],
          [24, "24 uur"],
        ],
      ]);
    for (const [key, title, choices] of fields) {
      const label = node("label", title),
        input = node("select");
      for (const [value, text] of choices) {
        const option = node("option", text);
        option.value = value;
        input.append(option);
      }
      input.value =
        tile.options?.[key] ??
        (key === "tap"
          ? "auto"
          : key === "display"
            ? "standard"
            : key === "history_hours"
              ? 24
              : "none");
      controls[key] = input;
      input.onchange = () => {
        tile.options = {
          ...tile.options,
          [key]: key === "history_hours" ? Number(input.value) : input.value,
        };
        if (key === "display" && input.value === "watch")
          tile.options.inline = "none";
        if (key === "inline" && input.value === "slider")
          tile.options.display = "standard";
        for (const [field, control] of Object.entries(controls)) {
          if (tile.options[field] !== undefined)
            control.value = tile.options[field];
        }
        markDirty();
        renderPreview();
      };
      label.append(input);
      options.append(label);
    }
    const inspectTile = node("button", "Inspecteer deze tegel", "quiet");
    inspectTile.onclick = () => inspect(tile.entity);
    options.append(inspectTile);
    content.append(options);
    li.append(domainBadge(tile.entity), content, actions);
    $("#tiles").append(li);
  });
}
function renderResults() {
  if (!layout) return;
  const query = $("#search").value.toLocaleLowerCase();
  const chosen = new Set(layout.tiles.map((t) => t.entity));
  const matches = inventory.entities.filter(
    (e) =>
      (!filter || e.id.startsWith(filter + ".") ||
        ({switch:"input_boolean", number:"input_number", select:"input_select"}[filter] === e.id.split(".")[0])) &&
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
    b.onclick = () => {
      layout.tiles.push({ entity: entity.id, name: "" });
      selectedTile = entity.id;
      markDirty();
      renderTiles();
      renderResults();
    };
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
async function refresh() {
  try {
    inventory = await (await api("inventory")).json();
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
async function checkWifiSecrets() {
  const fields = $("#wifi-fields"), status = $("#wifi-status");
  fields.hidden = true;
  fields.disabled = true;
  $("#create-profile").disabled = true;
  status.textContent = "ESPHome wifi-instellingen controleren…";
  try {
    const {wifi} = await (await api("firmware")).json();
    if (wifi?.state === "ready") {
      status.textContent = "✓ Bestaande ESPHome-wifi gevonden. Dit scherm gebruikt automatisch wifi_ssid en wifi_password uit je secrets.yaml.";
      $("#create-profile").disabled = false;
    } else if (wifi?.state === "new") {
      status.textContent = "Nog geen ESPHome-secrets gevonden. Vul wifi één keer in; volgende schermen gebruiken deze gegevens automatisch.";
      fields.hidden = false;
      fields.disabled = false;
      $("#create-profile").disabled = false;
    } else {
      status.textContent = "Controleer wifi_ssid en wifi_password in ESPHome secrets.yaml. Je bestaande bestand blijft behouden. Sluit en open deze wizard opnieuw na aanpassen.";
    }
  } catch {
    status.textContent = "ESPHome-secrets konden niet worden gecontroleerd. Sluit en open de wizard opnieuw om het nogmaals te proberen.";
  }
}
$("#new-screen").onclick = $("#start").onclick = () => {
  $("#installer").showModal();
  checkWifiSecrets();
};
$("#close-install").onclick = () => $("#installer").close();
let generatedName = "";
$("#install-form").onsubmit = async (e) => {
  e.preventDefault();
  $("#download").disabled = true;
  try {
    const data = Object.fromEntries(new FormData(e.target));
    const response = await api("install", {
      method: "POST",
      body: JSON.stringify(data),
    });
    $("#yaml-output").value = await response.text();
    generatedName = data.name;
    $("#yaml-result").hidden = false;
    $("#install-status").textContent =
      "Je YAML is klaar. Kopieer hem naar ESPHome en bewaar het bestand met sleutels.";
  } catch (e) {
    $("#install-status").textContent = e.message;
  } finally {
    $("#download").disabled = false;
  }
};
$("#copy-yaml").onclick = async () => {
  try {
    if (navigator.clipboard && window.isSecureContext)
      await navigator.clipboard.writeText($("#yaml-output").value);
    else {
      $("#yaml-output").select();
      if (!document.execCommand("copy")) throw new Error();
    }
    $("#install-status").textContent =
      "Gekopieerd. Plak dit bij Edit in ESPHome Device Builder.";
  } catch {
    $("#yaml-output").select();
    $("#install-status").textContent =
      "De tekst is geselecteerd. Kopieer met Ctrl+C of Command+C.";
  }
};
$("#download-yaml").onclick = () => {
  const url = URL.createObjectURL(
    new Blob([$("#yaml-output").value], { type: "text/plain" }),
  );
  const a = node("a");
  a.href = url;
  a.download = `${generatedName}.yaml`;
  document.body.append(a);
  a.click();
  a.remove();
  setTimeout(() => URL.revokeObjectURL(url), 30000);
  $("#install-status").textContent =
    "Download gestart. Blokkeert je browser deze? Gebruik Kopieer YAML.";
};
window.addEventListener("beforeunload", (e) => {
  if (dirty) {
    e.preventDefault();
    e.returnValue = "";
  }
});
refresh();
setInterval(refresh, 10000);

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
$("#create-profile").onclick = async () => {
  const form = $("#install-form");
  if (!form.reportValidity()) return;
  try {
    const data = await (
      await api("firmware/profiles", {
        method: "POST",
        body: JSON.stringify(Object.fromEntries(new FormData(form))),
      })
    ).json();
    generatedName = data.file.replace(/\.yaml$/, " ").trim();
    $("#yaml-output").value = data.yaml;
    $("#yaml-result").hidden = false;
    $("#install-status").textContent =
      `${data.file} bewaard. Open Firmware & USB om te installeren.`;
    form.elements.wifi_password.value = "";
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
          `Kleine slider: ${options.inline === "slider" ? "ja" : "nee"} · Weergave: ${options.display === "watch" ? "grote waarde" : "standaard"}`,
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
