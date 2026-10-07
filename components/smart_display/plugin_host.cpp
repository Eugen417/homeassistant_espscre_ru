// The plugin API's core side (plugin_api.h, plugin_host.h, docs/PLUGINS.md). Apart from main.cpp, as energy_view.cpp
// and page_receiver.cpp are: main.cpp's literal pool is close to the S3's l32r range.
#include "runtime_tiles.h"
#include "plugin_host.h"
#include <functional>
#include <memory>

namespace rt = runtime_tiles;

// ---- The register (plugin_api.h) ----
namespace tessera {

std::vector<Plugin *> &plugins() {
  static std::vector<Plugin *> list;
  return list;
}
std::vector<TileType> &tile_types() {
  static std::vector<TileType> list;
  return list;
}
const TileType *tile_type(const std::string &entity) {
  for (const auto &type : tile_types())
    if (type.entity == entity) return &type;
  return nullptr;
}

std::vector<CardType> &card_types() {
  static std::vector<CardType> list;
  return list;
}
std::vector<TapAction> &tap_actions() {
  static std::vector<TapAction> list;
  return list;
}

Plugin::Plugin() { plugins().push_back(this); }

void Plugin::add_card(const char *id, std::function<Card *()> make, bool wide) {
  std::string key = std::string("plugin:") + plugin_id() + "." + id;
  for (auto &type : card_types())
    if (type.key == key) { type.make = std::move(make); type.wide = wide; return; }
  card_types().push_back({this, id, std::move(key), wide, std::move(make)});
}

void Plugin::add_tap_action(const char *id, std::function<void(const TapContext &)> run) {
  std::string key = std::string("plugin:") + plugin_id() + "." + id;
  for (auto &action : tap_actions())
    if (action.key == key) { action.run = std::move(run); return; }
  tap_actions().push_back({this, id, std::move(key), std::move(run)});
}

void Plugin::add_tile(const char *id, std::function<Tile *()> make) {
  std::string entity = std::string("plugin:") + plugin_id() + "." + id;
  for (auto &type : tile_types())
    if (type.entity == entity) { type.make = std::move(make); return; }
  tile_types().push_back({this, id, std::move(entity), std::move(make)});
}

const char *Plugin::text(const char *key) const {
  for (const auto &pair : texts_)
    if (strcmp(pair.first, key) == 0) return pair.second;
  return "";
}

uint16_t Plugin::memory(const std::string &tile) const {
  for (const auto &pair : memory_)
    if (tile == pair.first) return pair.second;
  return 1024;
}

SettingsPage &SettingsPage::toggle(const char *label, std::function<bool()> read, std::function<void(bool)> write) {
  Item item{Item::TOGGLE};
  item.label = label ? label : "";
  item.read = [read]() { return read && read() ? 1 : 0; };
  item.write = [write](int on) { if (write) write(on != 0); };
  items.push_back(std::move(item));
  return *this;
}
SettingsPage &SettingsPage::number(const char *label, int low, int high, int step, const char *unit,
                                   std::function<int()> read, std::function<void(int)> write) {
  Item item{Item::NUMBER};
  item.label = label ? label : "";
  item.low = low; item.high = high; item.step = step > 0 ? step : 1;
  item.unit = unit ? unit : "";
  item.read = std::move(read); item.write = std::move(write);
  items.push_back(std::move(item));
  return *this;
}
SettingsPage &SettingsPage::choice(const char *label, std::vector<std::string> options, std::function<int()> read,
                                   std::function<void(int)> write) {
  Item item{Item::CHOICE};
  item.label = label ? label : "";
  item.options = std::move(options);
  item.read = std::move(read); item.write = std::move(write);
  items.push_back(std::move(item));
  return *this;
}
SettingsPage &SettingsPage::action(const char *label, const char *icon, std::function<void()> run, const char *confirm) {
  Item item{Item::ACTION};
  item.label = label ? label : "";
  item.icon = icon ? icon : "";
  item.confirm = confirm ? confirm : "";
  item.run = std::move(run);
  items.push_back(std::move(item));
  return *this;
}
SettingsPage &SettingsPage::info(const char *label, std::function<std::string()> text) {
  Item item{Item::INFO};
  item.label = label ? label : "";
  item.text = std::move(text);
  items.push_back(std::move(item));
  return *this;
}
SettingsPage &SettingsPage::card(const char *label, const char *icon, const char *card) {
  Item item{Item::CARD};
  item.label = label ? label : "";
  item.icon = icon ? icon : "";
  item.card = card ? card : "";
  items.push_back(std::move(item));
  return *this;
}

uint32_t epoch() { return rt::now_epoch(); }

LocalTime local_time(uint32_t when) {
  const esphome::ESPTime t = esphome::ESPTime::from_epoch_local(when);
  return {t.year, t.month, t.day_of_month, t.hour, t.minute, t.second, t.day_of_week};
}

std::string clock_text(uint32_t when) {
  if (!when) return "";
  const esphome::ESPTime moment = esphome::ESPTime::from_epoch_local(when);
  return screen_text::clock_text(rt::hhmm(moment), screen_settings::current.clock_24h != 0);
}

std::string format(const char *text, long n) {
  std::string all = text ? text : "", form = all;
  if (all.find('|') != std::string::npos) {
    int wanted = screen_text::plural_index(static_cast<int>(n));
    size_t start = 0;
    for (int i = 0; i < wanted; ++i) {
      size_t bar = all.find('|', start);
      if (bar == std::string::npos) break;
      start = bar + 1;
    }
    size_t end = all.find('|', start);
    form = all.substr(start, end == std::string::npos ? std::string::npos : end - start);
    size_t first = form.find_first_not_of(' '), last = form.find_last_not_of(' ');
    form = first == std::string::npos ? std::string() : form.substr(first, last - first + 1);
  }
  return screen_text::fill(form, "n", std::to_string(n));
}

std::string fill(const char *text, const char *name, const std::string &value) {
  return screen_text::fill(std::string(text ? text : ""), name, value);
}

bool action(const char *service, const std::string &entity, const char *key, const std::string &value) {
  if (!service || !rt::valid_entity(entity) || entity.rfind("plugin:", 0) == 0) return false;
  return rt::action(service, entity, key ? key : "", value);
}

bool open_card(const char *plugin_id, const char *card, const std::string &entity, int tile, const std::string &title) {
  return plugin_host::open_card(std::string("plugin:") + (plugin_id ? plugin_id : "") + "." + (card ? card : ""), entity,
                                tile, title);
}

void close_card() { plugin_host::close_card(); }

void refresh() {
  for (auto &w : rt::widgets)
    if (w.plugin && w.index < rt::model.count) rt::mark_tile(w.index);
  if (rt::refresh) rt::refresh();
}

namespace ui {
const lv_font_t *font(Font f) {
  const lv_font_t *title = nullptr, *value = nullptr;
  for (auto &w : rt::widgets) {
    if (!title && w.title_font) title = w.title_font;
    if (!value && w.value_font) value = w.value_font;
  }
  switch (f) {
    case Font::VALUE: return rt::watch_value_font ? rt::watch_value_font : rt::watch_font;
    case Font::HEADLINE: return rt::watch_font ? rt::watch_font : title;
    case Font::TITLE: return title ? title : rt::control_font;
    case Font::BODY_LARGE: return rt::control_font ? rt::control_font : value;
    case Font::BODY: return value ? value : rt::small_font;
    case Font::ICON: return rt::tile_icon_font();
    case Font::ICON_SMALL: return rt::mini_icon_font ? rt::mini_icon_font : rt::tile_icon_font();
  }
  return value;
}

int text_width(const std::string &text, Font f) {
  const lv_font_t *face = font(f);
  return face ? rt::text_width(text, face) : 0;
}

lv_obj_t *label(lv_obj_t *parent, Font f, theme::Role role) {
  lv_obj_t *l = lv_label_create(parent);
  lv_obj_remove_flag(l, LV_OBJ_FLAG_CLICKABLE);
  lv_label_set_long_mode(l, LV_LABEL_LONG_DOT);
  lv_label_set_text(l, "");
  if (const lv_font_t *face = font(f)) lv_obj_set_style_text_font(l, face, 0);
  lv_obj_set_style_text_color(l, theme::color(role), 0);
  return l;
}

void set_text(lv_obj_t *l, const std::string &text) { if (l) rt::label(l, text); }
void set_font(lv_obj_t *l, Font f) {
  if (const lv_font_t *face = font(f)) if (l) rt::set_font(l, face);
}
void set_color(lv_obj_t *l, theme::Role role) {
  if (l) rt::set_color(l, LV_STYLE_TEXT_COLOR, theme::color(role));
}

lv_obj_t *block(lv_obj_t *parent, theme::Role fill) {
  lv_obj_t *b = lv_obj_create(parent);
  lv_obj_remove_style_all(b);
  lv_obj_remove_flag(b, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_remove_flag(b, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(b, theme::color(fill), 0);
  lv_obj_set_style_radius(b, ::ui::px(::ui::large() ? 12 : 8), 0);
  return b;
}

std::string icon(uint32_t codepoint) { return tile_icon::utf8(codepoint); }
}  // namespace ui

}  // namespace tessera

// ---- Plugin tiles in cards (plugin_host.h) ----
namespace plugin_host {

struct Card {
  std::unique_ptr<tessera::Tile> tile;
  std::string entity;          // what the card was made for: the tile type,
  size_t index = SIZE_MAX;     // the tile of the layout,
  int width = 0, height = 0;   // its room,
  size_t options = 0;          // and its options (a hash)
  size_t state = 0;            // the data it was last given (a hash), 0 for none yet
  bool dark = false;           // the look it last coloured itself for
};

static size_t hash(const std::string &text) { return std::hash<std::string>{}(text) | 1; }

bool known(const rt::Tile &t) { return tessera::tile_type(t.entity) != nullptr; }

void release(rt::Widgets &w) {
  if (!w.plugin) return;
  delete w.plugin;  // the plugin's parts are the extra layer's children: whoever releases the card cleans that
  w.plugin = nullptr;
}

void render(rt::Widgets &w, const rt::Tile &t, int width, int height) {
  const tessera::TileType *type = tessera::tile_type(t.entity);
  if (!type || !type->make) return;
  rt::begin_extra(w, "plugin", width, height);
  const auto &extra = t.extra();
  const size_t options = hash(extra.plugin_options + "|" + extra.plugin_entity);
  Card *card = w.plugin;
  if (card && (card->entity != t.entity || card->index != w.index || card->width != width || card->height != height ||
               card->options != options)) {
    release(w);
    lv_obj_clean(w.extra);
    card = nullptr;
  }
  if (!card) {
    lv_obj_clean(w.extra);
    std::unique_ptr<tessera::Tile> tile(type->make());
    if (!tile) return;
    card = w.plugin = new Card();
    card->tile = std::move(tile);
    card->entity = t.entity;
    card->index = w.index;
    card->width = width;
    card->height = height;
    card->options = options;
    card->dark = theme::dark;
    JsonDocument doc;
    if (extra.plugin_options.empty() || deserializeJson(doc, extra.plugin_options)) doc.to<JsonObject>();
    tessera::TileContext context{w.extra, width, height, static_cast<uint8_t>(t.column_span()),
                                 static_cast<uint8_t>(t.row_span()), t.name.c_str(), extra.plugin_entity.c_str(),
                                 static_cast<int>(w.index), doc.as<JsonObjectConst>()};
    card->tile->create(context);
  }
  const size_t state = hash(extra.plugin_state);
  if (card->state != state) {
    card->state = state;
    JsonDocument doc;
    if (extra.plugin_state.empty() || deserializeJson(doc, extra.plugin_state)) doc.to<JsonObject>();
    card->tile->on_state(doc.as<JsonObjectConst>());
    card->tile->on_tick(rt::now_epoch());
  }
  if (card->dark != theme::dark) {
    card->dark = theme::dark;
    card->tile->on_theme();
  }
}

void tap(rt::Widgets &w) {
  if (w.plugin && w.plugin->tile) w.plugin->tile->on_tap();
}

void tick_cards(uint32_t epoch) {
  tick_card(epoch);
  for (auto &w : rt::widgets) {
    if (!w.plugin || !w.tile || !w.extra || w.extra_mode != "plugin" || w.index >= rt::model.count) continue;
    if (lv_obj_has_flag(w.tile, LV_OBJ_FLAG_HIDDEN) || lv_obj_has_flag(w.extra, LV_OBJ_FLAG_HIDDEN)) continue;
    w.plugin->tile->on_tick(epoch);
  }
}

uint16_t bytes(const std::string &entity) {
  const tessera::TileType *type = tessera::tile_type(entity);
  return type ? type->plugin->memory(type->id) : PLACEHOLDER_BYTES;
}

// ---- A plugin's card over the page ----
struct OpenCard {
  std::unique_ptr<tessera::Card> card;
  std::string key, entity;
  int tile = -1;
  size_t state = 0;
  bool dark = false;
  lv_obj_t *backdrop = nullptr, *root = nullptr;
};
static OpenCard *shown = nullptr;
static bool closing = false;

// What the tile a card was opened from says now: a plugin tile's data, or an entity's state and name.
static std::string card_data(int tile) {
  if (tile < 0 || static_cast<size_t>(tile) >= rt::model.count) return "";
  const auto &t = rt::model.tiles[tile];
  if (t.is_plugin()) return t.extra().plugin_state;
  JsonDocument doc;
  doc["state"] = t.state;
  doc["name"] = t.name;
  std::string out;
  serializeJson(doc, out);
  return out;
}

static void card_state(OpenCard &open, bool force) {
  const std::string data = card_data(open.tile);
  const size_t state = hash(data);
  if (!force && state == open.state) return;
  open.state = state;
  JsonDocument doc;
  if (data.empty() || deserializeJson(doc, data)) doc.to<JsonObject>();
  open.card->on_state(doc.as<JsonObjectConst>());
}

bool card_open() { return shown != nullptr; }

void close_card() {
  if (!shown || closing) return;
  closing = true;
  OpenCard *open = shown;
  shown = nullptr;
  open->card.reset();  // its parts are the root's children: deleted with it, never by the card
  if (open->root) lv_obj_delete(open->root);
  if (open->backdrop) lv_obj_delete(open->backdrop);
  delete open;
  closing = false;
}

bool open_card(const std::string &key, const std::string &entity, int tile, const std::string &title) {
  const tessera::CardType *type = nullptr;
  for (const auto &t : tessera::card_types())
    if (t.key == key) type = &t;
  if (!type || !type->make) return false;
  // One card at a time, as Tessera's own: the one open now, and a detail card of a tile, go first.
  close_card();
  if (rt::dismiss) rt::dismiss();
  std::unique_ptr<tessera::Card> card(type->make());
  if (!card) return false;
  auto *open = new OpenCard();
  open->card = std::move(card);
  open->key = key;
  open->entity = entity;
  open->tile = tile;
  open->dark = theme::dark;
  const bool large = ::ui::large();
  // The page's ground over everything, taking every press so nothing reaches the page under it.
  open->backdrop = lv_obj_create(lv_screen_active());
  lv_obj_remove_style_all(open->backdrop);
  lv_obj_set_size(open->backdrop, lv_pct(100), lv_pct(100));
  lv_obj_remove_flag(open->backdrop, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(open->backdrop, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_bg_color(open->backdrop, theme::color(theme::PAGE), 0);
  lv_obj_set_style_bg_opa(open->backdrop, LV_OPA_COVER, 0);
  open->root = lv_obj_create(lv_screen_active());
  lv_obj_remove_style_all(open->root);
  lv_obj_remove_flag(open->root, LV_OBJ_FLAG_SCROLLABLE);
  const auto kind = type->wide ? overlay_card::picture : overlay_card::controls;
  overlay_card::frame(open->root, kind, 1);
  const int width = overlay_card::content_width(kind, 1), height = overlay_card::screen_height();
  // The same top bar as Tessera's cards: a round back key at the left, the title in the middle.
  const int bar = ::ui::px(large ? 60 : 40), bar_x = ::ui::px(large ? 16 : 10), bar_y = ::ui::px(large ? 16 : 8);
  auto *back = lv_obj_create(open->root);
  lv_obj_remove_style_all(back);
  lv_obj_set_pos(back, bar_x, bar_y);
  lv_obj_set_size(back, bar, bar);
  lv_obj_set_style_radius(back, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_opa(back, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(back, theme::color(theme::KEY), 0);
  lv_obj_set_style_bg_color(back, theme::color(theme::KEY_PRESSED), LV_STATE_PRESSED);
  lv_obj_add_flag(back, LV_OBJ_FLAG_CLICKABLE);
  auto *arrow = lv_label_create(back);
  if (rt::mini_icon_font) lv_obj_set_style_text_font(arrow, rt::mini_icon_font, 0);
  lv_obj_set_style_text_color(arrow, theme::color(theme::INK), 0);
  lv_label_set_text(arrow, "\U000F004D");
  lv_obj_center(arrow);
  lv_obj_add_event_cb(back, [](lv_event_t *) {
    if (!shown || !rt::allowed(esphome::millis(), 14, "plugin card back")) return;
    if (!shown->card->on_back()) close_card();
  }, LV_EVENT_SHORT_CLICKED, nullptr);
  const lv_font_t *title_font = rt::watch_font ? rt::watch_font : tessera::ui::font(tessera::Font::TITLE);
  std::string words = title;
  if (words.empty() && tile >= 0 && static_cast<size_t>(tile) < rt::model.count) words = rt::model.tiles[tile].name;
  auto *heading = lv_label_create(open->root);
  lv_label_set_long_mode(heading, LV_LABEL_LONG_DOT);
  lv_label_set_text(heading, words.c_str());
  if (title_font) lv_obj_set_style_text_font(heading, title_font, 0);
  lv_obj_set_style_text_color(heading, theme::color(theme::INK), 0);
  lv_obj_set_style_text_align(heading, LV_TEXT_ALIGN_CENTER, 0);
  const int line = title_font ? lv_font_get_line_height(title_font) : bar;
  lv_obj_set_pos(heading, bar_x + bar + 8, bar_y + (bar - line) / 2);
  lv_obj_set_size(heading, std::max(1, width - 2 * (bar_x + bar + 8)), line);
  // The card's own room, under the bar, with the card's padding at the sides and the foot.
  const int pad = overlay_card::pad(), top = bar_y + bar + ::ui::px(large ? 12 : 6);
  auto *area = lv_obj_create(open->root);
  lv_obj_remove_style_all(area);
  lv_obj_remove_flag(area, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_pos(area, pad, top);
  lv_obj_set_size(area, std::max(1, width - 2 * pad), std::max(1, height - top - pad));
  shown = open;
  tessera::CardContext context{area, std::max(1, width - 2 * pad), std::max(1, height - top - pad), open->entity.c_str(),
                               tile};
  open->card->open(context);
  card_state(*open, true);
  open->card->on_tick(rt::now_epoch());
  return true;
}

void tick_card(uint32_t epoch) {
  if (!shown) return;
  card_state(*shown, false);
  if (shown->dark != theme::dark) {
    shown->dark = theme::dark;
    shown->card->on_theme();
  }
  shown->card->on_tick(epoch);
}

bool tap_action(size_t index) {
  if (index >= rt::model.count) return false;
  const auto &t = rt::model.tiles[index];
  for (const auto &action : tessera::tap_actions())
    if (action.key == t.tap && action.run) {
      tessera::TapContext context{t.entity.c_str(), t.name.c_str(), static_cast<int>(index)};
      action.run(context);
      return true;
    }
  return false;
}

// ---- The plugins' pages on the settings page ----
// Built once: the rows point into these, so nothing here moves afterwards.
struct SettingsStore {
  std::vector<std::unique_ptr<tessera::SettingsPage>> pages;   // what each plugin added
  std::vector<std::unique_ptr<std::vector<settings_screen::Row>>> rows;
  std::vector<std::unique_ptr<std::vector<const char *>>> options;
  std::vector<std::pair<tessera::Plugin *, std::string>> cards;  // a card row: its plugin and card
};
static SettingsStore &settings_store() {
  static SettingsStore store;
  return store;
}

static void build_settings() {
  auto &store = settings_store();
  std::vector<std::pair<tessera::Plugin *, tessera::SettingsPage *>> added;
  for (auto *p : tessera::plugins()) {
    auto page = std::make_unique<tessera::SettingsPage>();
    if (!p->settings(*page) || page->items.empty()) continue;
    if (page->title.empty()) page->title = *p->text("name") ? p->text("name") : p->plugin_id();
    if (page->icon.empty()) page->icon = "\U000F0A66";
    added.push_back({p, page.get()});
    store.pages.push_back(std::move(page));
  }
  if (added.empty()) return;
  using settings_screen::Row;
  using settings_screen::Kind;
  // The list of plugins: one row per plugin, opening its page.
  auto list = std::make_unique<std::vector<Row>>();
  for (size_t i = 0; i < added.size(); ++i) {
    Row row{};
    row.kind = Kind::page;
    row.words = added[i].second->title.c_str();
    row.icon = added[i].second->icon.c_str();
    row.opens = static_cast<uint8_t>(settings_screen::PLUGINS_PAGE + 1 + i);
    list->push_back(row);
  }
  settings_screen::plugin_pages.push_back({screen_text::txt::settings_plugins, list->data(),
                                           static_cast<uint8_t>(std::min<size_t>(list->size(), 12)), 0, nullptr});
  store.rows.push_back(std::move(list));
  for (auto &[plugin, page] : added) {
    auto rows = std::make_unique<std::vector<Row>>();
    for (auto &item : page->items) {
      Row row{};
      row.words = item.label.c_str();
      row.ctx = &item;
      using Item = tessera::SettingsPage::Item;
      switch (item.kind) {
        case Item::TOGGLE: row.kind = Kind::toggle; break;
        case Item::NUMBER:
          row.kind = Kind::number; row.low = item.low; row.high = item.high; row.step = item.step;
          row.unit = item.unit.c_str();
          break;
        case Item::CHOICE: {
          row.kind = Kind::choice;
          auto words = std::make_unique<std::vector<const char *>>();
          for (auto &option : item.options) words->push_back(option.c_str());
          row.options = words->data();
          row.option_count = static_cast<uint8_t>(std::min<size_t>(words->size(), 255));
          store.options.push_back(std::move(words));
          break;
        }
        case Item::ACTION:
        case Item::CARD:
          row.kind = Kind::action;
          row.icon = item.icon.c_str();
          if (!item.confirm.empty()) row.confirm_words = item.confirm.c_str();
          break;
        case Item::INFO: row.kind = Kind::info; break;
      }
      if (item.kind == Item::CARD) {
        store.cards.push_back({plugin, item.card});
        item.run = [plugin = plugin, card = item.card]() {
          settings_screen::close();
          tessera::open_card(plugin->plugin_id(), card.c_str());
        };
      }
      row.read_ctx = [](void *c) -> int32_t { auto *i = static_cast<Item *>(c); return i->read ? i->read() : 0; };
      row.write_ctx = [](void *c, int32_t v) { auto *i = static_cast<Item *>(c); if (i->write) i->write(v); };
      row.run_ctx = [](void *c) { auto *i = static_cast<Item *>(c); if (i->run) i->run(); };
      row.text_ctx = [](void *c) -> std::string { auto *i = static_cast<Item *>(c); return i->text ? i->text() : ""; };
      rows->push_back(row);
      if (rows->size() == 12) break;   // what one page draws (settings_screen::draw)
    }
    settings_screen::plugin_pages.push_back({0, rows->data(), static_cast<uint8_t>(rows->size()),
                                             settings_screen::PLUGINS_PAGE, page->title.c_str()});
    store.rows.push_back(std::move(rows));
  }
}

void ready() {
  build_settings();
  for (auto *p : tessera::plugins()) p->on_ready();
}
void tick(uint32_t now_ms, bool dimmed) {
  static bool was_dimmed = false;
  if (dimmed != was_dimmed) {
    was_dimmed = dimmed;
    standby(dimmed);
  }
  for (auto *p : tessera::plugins()) p->on_tick(now_ms);
}
void standby(bool dark) { for (auto *p : tessera::plugins()) p->on_standby(dark); }
void before_update() { for (auto *p : tessera::plugins()) p->before_update(); }

void hello(JsonObject root) {
  char api[8];
  snprintf(api, sizeof api, "%u.%u", tessera::PLUGIN_API_MAJOR, tessera::PLUGIN_API_MINOR);
  root["plugin_api"] = std::string(api);
  auto list = root["plugins"].to<JsonArray>();
  for (auto *p : tessera::plugins()) {
    auto item = list.add<JsonObject>();
    item["id"] = std::string(p->plugin_id());
    item["version"] = std::string(p->plugin_version());
    auto tiles = item["tiles"].to<JsonArray>();
    for (const auto &type : tessera::tile_types())
      if (type.plugin == p) tiles.add(type.id);
    auto taps = item["taps"].to<JsonArray>();
    for (const auto &action : tessera::tap_actions())
      if (action.plugin == p) taps.add(action.id);
  }
}

}  // namespace plugin_host
