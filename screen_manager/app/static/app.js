"use strict";
const $ = (s) => document.querySelector(s);
let inventory = { screens: [], entities: [] },
  selected = null,
  layout = null,
  dirty = false,
  filter = "",
  dragIndex = -1,
  busy = false;
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
function renderTiles() {
  $("#tiles").replaceChildren();
  $("#count").textContent = `${layout.tiles.length} / 10`;
  $("#no-tiles").hidden = layout.tiles.length > 0;
  layout.tiles.forEach((tile, i) => {
    if (i === 6) $("#tiles").append(node("li", "Pagina 2", "page-break"));
    const li = node("li", undefined, "tile");
    li.draggable = true;
    li.ondragstart = (e) => {
      if (e.target instanceof HTMLInputElement) {
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
    li.append(node("span", "⠿", "grip"), content, actions);
    $("#tiles").append(li);
  });
}
function renderResults() {
  if (!layout) return;
  const query = $("#search").value.toLocaleLowerCase();
  const chosen = new Set(layout.tiles.map((t) => t.entity));
  const matches = inventory.entities.filter(
    (e) =>
      (!filter || e.id.startsWith(filter + ".")) &&
      `${e.name} ${e.id} ${e.device} ${e.area}`
        .toLocaleLowerCase()
        .includes(query),
  );
  $("#results").replaceChildren();
  for (const entity of matches.slice(0, 80)) {
    const b = node("button", undefined, "result"),
      description = node("span");
    description.append(
      node("span", entity.name),
      node("small", [entity.area, entity.device].filter(Boolean).join(" · ")),
      node("small", entity.id),
    );
    b.append(
      description,
      node("span", chosen.has(entity.id) ? "✓" : "+", "plus"),
    );
    b.disabled = chosen.has(entity.id) || layout.tiles.length >= 10;
    b.onclick = () => {
      layout.tiles.push({ entity: entity.id, name: "" });
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
    if (!selected && inventory.screens.length) select(inventory.screens[0].id);
  } catch {
    $("#connection").textContent =
      "Beheerpagina niet bereikbaar · opnieuw proberen…";
  }
}
$("#save").onclick = async () => {
  if (busy) return;
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
    busy = false;
    $("#save").disabled = false;
  }
};
$("#title").oninput = markDirty;
$("#search").oninput = renderResults;
$("#refresh").onclick = refresh;
for (const [value, label] of [
  ["", "Alles"],
  ["light", "Lampen"],
  ["climate", "Klimaat"],
  ["scene", "Scènes"],
  ["vacuum", "Vacuum"],
  ["sensor", "Sensoren"],
]) {
  const b = node("button", label, value === "" ? "active" : "");
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
$("#new-screen").onclick = $("#start").onclick = () =>
  $("#installer").showModal();
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
