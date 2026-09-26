// clang++ -std=c++17 -Wall -Wextra -Werror -I. tests/test_picture_store.cpp -o /tmp/test_picture_store && /tmp/test_picture_store
#include "../components/smart_display/picture_store.h"
#include <cassert>
#include <cstdlib>
#include <set>
#include <vector>

// LVGL's image descriptor, as far as the store reads it.
struct Header { uint32_t w = 0, h = 0, cf = 0; };
struct Image { Header header; uint32_t data_size = 0; const uint8_t *data = nullptr; };

int main() {
  int live = 0;
  picture_store::Store<Image> store;
  store.allocate = [&](size_t n) { ++live; return std::malloc(n); };
  store.release = [&](void *p) { --live; std::free(p); };

  std::vector<uint8_t> red(100, 1), blue(100, 2), big(300, 3);
  Image a{{10, 5, 1}, 100, red.data()};

  // No budget, no pictures: a board without PSRAM draws from the download as before.
  assert(!store.put("cover", a, 1));
  store.budget = 1000;

  // A copy of its own, found again by what was asked for.
  Image *kept = store.put("cover", a, 1, "note");
  assert(kept && kept->data != red.data() && kept->data[0] == 1 && store.find("cover") == kept && live == 1);
  assert(store.entry("cover")->note == "note" && !store.find("other"));

  // The next camera picture of the same size is written over the old one: the card keeps its pointer.
  Image b{{10, 5, 1}, 100, blue.data()};
  assert(store.put("cover", b, 2) == kept && kept->data[0] == 2 && live == 1 && store.entry("cover")->stored_at == 2);

  // Another size: a new copy, the old one waits while a card still shows it.
  std::set<const Image *> shown{kept};
  auto on_card = [&](const Image *i) { return shown.count(i) > 0; };
  Image c{{10, 15, 1}, 300, big.data()};
  Image *bigger = store.put("cover", c, 3);
  assert(bigger && bigger != kept && store.find("cover") == bigger && live == 2);
  store.collect(on_card);
  assert(live == 2);
  shown = {bigger};
  store.collect(on_card);
  assert(live == 1 && store.size() == 300);

  // Over budget: the picture used longest ago goes, never one on a card.
  store.budget = 500;
  assert(store.put("a", a, 4) && store.put("b", b, 5));  // 300 + 100 + 100
  store.find("a");                                        // of the two not on a card, "b" was used longest ago
  store.budget = 350;
  store.collect(on_card);                                 // "cover" is on a card and stays: "b" goes, then "a"
  assert(store.find("cover") && !store.find("b") && !store.find("a") && live == 1);

  // A picture larger than the budget is not kept.
  store.budget = 200;
  assert(!store.put("huge", c, 6));

  // Another layout: what no card shows goes now, the rest once its card lets it go.
  store.budget = 1000;
  store.put("x", a, 7);
  store.forget(on_card);
  assert(!store.find("x") && !store.find("cover") && live == 1);
  shown.clear();
  store.collect(on_card);
  assert(live == 0 && store.size() == 0);
  return 0;
}
