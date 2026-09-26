#pragma once
// Pictures kept until they change (firmware 0.3.2+). A download lands in one of the board's online_image buffers, which
// the next download overwrites; the store copies it into memory of its own (PSRAM) under what was asked for (the tiles,
// their colours, the track), and the cards draw that copy. A page that comes back finds its pictures here: an album cover
// is not fetched again while the track plays, a camera shows its last picture at once and refreshes at its own pace.
//
// A copy stays where it is while a card may draw it: a camera's next picture of the same size is written over the old
// one, another size gets a new copy and the old one waits until no card shows it (`collect`). The store keeps to its
// budget by dropping the pictures used longest ago, never one a card still shows.
//
// Pure bookkeeping over an image descriptor with LVGL's fields (header, data_size, data), so tests/test_picture_store.cpp
// checks it on a PC with a stand-in; runtime_tiles.h uses lv_image_dsc_t and PSRAM.
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <string>

namespace picture_store {
constexpr size_t ENTRIES = 16;

template <class Image> struct Store {
  struct Entry {
    std::string key, note;   // note: what the picture holds beyond its key (the tiles a strip answered for)
    Image image{};
    void *buffer = nullptr;
    size_t bytes = 0;
    uint32_t stored_at = 0, used = 0;
    bool retired = false;    // replaced by a copy of another size; freed once no card shows it
  };
  std::array<Entry, ENTRIES> entries{};
  std::function<void *(size_t)> allocate;
  std::function<void(void *)> release;
  size_t budget = 0;         // bytes; 0 keeps nothing, so a board without the room stores no picture
  uint32_t uses = 0;

  size_t size() const {
    size_t total = 0;
    for (const auto &e : entries) total += e.bytes;
    return total;
  }
  Entry *entry(const std::string &key) {
    if (key.empty()) return nullptr;
    for (auto &e : entries) if (e.buffer && !e.retired && e.key == key) return &e;
    return nullptr;
  }
  // The kept picture for `key`, or nullptr.
  Image *find(const std::string &key) {
    Entry *e = entry(key);
    if (!e) return nullptr;
    e->used = ++uses;
    return &e->image;
  }
  // Keep a copy of `from` under `key`. The same key with the same size keeps its place (the cards drawing it show the
  // new picture); nullptr when there is no room, and the caller draws nothing from the store.
  Image *put(const std::string &key, const Image &from, uint32_t now, const std::string &note = {}) {
    if (!budget || !allocate || !from.data || !from.data_size || key.empty() || from.data_size > budget) return nullptr;
    Entry *e = entry(key);
    if (e && e->bytes == from.data_size && std::memcmp(&e->image.header, &from.header, sizeof(from.header)) == 0) {
      std::memcpy(e->buffer, from.data, from.data_size);
      e->stored_at = now; e->used = ++uses; e->note = note;
      return &e->image;
    }
    if (e) e->retired = true;
    Entry *slot = nullptr;
    for (auto &c : entries) if (!c.buffer) { slot = &c; break; }
    if (!slot) return nullptr;
    void *buffer = allocate(from.data_size);
    if (!buffer) return nullptr;
    std::memcpy(buffer, from.data, from.data_size);
    slot->key = key; slot->note = note; slot->buffer = buffer; slot->bytes = from.data_size;
    slot->image = from;
    slot->image.data = static_cast<decltype(from.data)>(buffer);
    slot->stored_at = now; slot->used = ++uses; slot->retired = false;
    return &slot->image;
  }
  // Frees what may go: copies that were replaced, then the pictures used longest ago while the store is over its
  // budget. `shown` says whether a card (or an open card over the page) still draws a picture.
  void collect(const std::function<bool(const Image *)> &shown) {
    for (auto &e : entries) if (e.buffer && e.retired && !shown(&e.image)) drop(e);
    while (size() > budget) {
      Entry *oldest = nullptr;
      for (auto &e : entries)
        if (e.buffer && !shown(&e.image) && (!oldest || e.used < oldest->used)) oldest = &e;
      if (!oldest) return;  // everything left is on a card: it stays until the card lets it go
      drop(*oldest);
    }
  }
  // Nothing kept is right any more (another layout, another look): what no card shows goes now, the rest as soon as
  // its card lets it go.
  void forget(const std::function<bool(const Image *)> &shown) {
    for (auto &e : entries) if (e.buffer && !shown(&e.image)) drop(e);
    for (auto &e : entries) if (e.buffer) e.retired = true;
  }

 private:
  void drop(Entry &e) {
    if (release) release(e.buffer);
    e = Entry{};
  }
};
}  // namespace picture_store
