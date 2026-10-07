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

Plugin::Plugin() { plugins().push_back(this); }

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
                                 doc.as<JsonObjectConst>()};
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

void ready() { for (auto *p : tessera::plugins()) p->on_ready(); }
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
  }
}

}  // namespace plugin_host
