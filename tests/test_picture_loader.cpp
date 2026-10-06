// clang++ -std=c++17 -Wall -Wextra -Werror -I. tests/test_picture_loader.cpp -o /tmp/test_picture_loader && /tmp/test_picture_loader
#include "../components/smart_display/picture_loader.h"
#include <cassert>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

using namespace picture_loader;

// A screen as the loader sees it: a store, the app's questions, the downloads and what owners were told.
struct Bench {
  Loader loader;
  std::map<std::string, uint32_t> store;           // key -> stored at
  std::vector<std::string> asked, loads, cancels, forgotten;
  std::vector<std::pair<std::string, Outcome>> told;
  Bench() {
    loader.kept = [this](const std::string &key, uint32_t &at) {
      auto it = store.find(key);
      if (it == store.end()) return false;
      at = it->second;
      return true;
    };
    loader.load = [this](Slot slot, const std::string &url, const std::string &) { loads.push_back(std::to_string(int(slot)) + ":" + url); };
    loader.cancel = [this](Slot slot) { cancels.push_back(std::to_string(int(slot))); };
    loader.forget = [this](const std::string &key) { forgotten.push_back(key); store.erase(key); };
  }
  Want want(const std::string &key, const std::string &tag, Rank rank, Slot slot = Slot::FULL, uint32_t every = 0, bool drawn = false) {
    Want w;
    w.key = key; w.tag = tag; w.rank = rank; w.slot = slot; w.every = every; w.drawn = drawn;
    w.ask = [this, tag] { asked.push_back(tag); };
    w.done = [this, key](Outcome o) { told.emplace_back(key, o); };
    return w;
  }
  // A download ends: into the store, and its owner is told.
  void finish(Slot slot, bool ok, uint32_t now) {
    const std::string key = loader.loaded(slot, ok, now);
    if (!key.empty()) { store[key] = now; loader.arrived(key, false); }
  }
};

static const Owner CARD{Kind::CARD, 0}, VIEW{Kind::VIEW, 0}, ALERT{Kind::ALERT, 0};
static Owner page(uint32_t n) { return {Kind::PAGE, n}; }
static Owner tile(uint32_t n) { return {Kind::TILE, n}; }

int main() {
  // A card's cover: asked for at once, loaded once its link is here, and then it is simply there.
  {
    Bench b;
    b.loader.begin(); b.loader.want(CARD, b.want("cover|p|a|132", "cover|p", Rank::CARD)); b.loader.end(1000, true);
    assert(b.asked.size() == 1 && b.loads.empty() && b.loader.phase(CARD) == Loader::Phase::ASKED);
    b.loader.answer("cover|p", "http://app/1.bmp", 1100);
    b.loader.begin(); b.loader.want(CARD, b.want("cover|p|a|132", "cover|p", Rank::CARD)); b.loader.end(1150, true);
    assert(b.loads.size() == 1 && b.loads[0] == "0:http://app/1.bmp" && b.loader.busy());
    b.finish(Slot::FULL, true, 1400);
    assert(b.told.size() == 1 && b.told[0].second == Outcome::LOADED && !b.loader.busy());
    for (uint32_t t = 1500; t < 60000; t += 250) {
      b.loader.begin(); b.loader.want(CARD, b.want("cover|p|a|132", "cover|p", Rank::CARD)); b.loader.end(t, true);
    }
    assert(b.asked.size() == 1 && b.loads.size() == 1 && b.loader.phase(CARD) == Loader::Phase::DONE);
    // The store let it go (room for another picture): fetched again at once through its link, never waited for.
    b.store.clear();
    b.loader.begin(); b.loader.want(CARD, b.want("cover|p|a|132", "cover|p", Rank::CARD)); b.loader.end(60000, true);
    assert(b.loads.size() == 2 && b.asked.size() == 1);
  }
  // A picture the store holds is there at once: nothing is asked, nothing loads.
  {
    Bench b;
    b.store["live|page2"] = 5;
    b.loader.begin(); b.loader.want(page(2), b.want("live|page2", "live|p", Rank::PAGE, Slot::LIVE)); b.loader.end(1000, true);
    assert(b.asked.empty() && b.loads.empty() && b.loader.settled_for(page(2)) && !b.loader.glass_waiting(1000));
  }
  // Pages turned fast: each page on the glass for a moment. What was not wanted any more never loads, a download for a
  // page already gone breaks off, and only the page that stays is loaded.
  {
    Bench b;
    uint32_t t = 1000;
    for (uint32_t n : {3u, 5u, 9u, 4u}) {
      b.loader.begin(); b.loader.want(page(n), b.want("live|page" + std::to_string(n), "live|" + std::to_string(n), Rank::PAGE, Slot::LIVE));
      b.loader.end(t, false);  // the pages do not stand still: nothing starts loading
      b.loader.answer("live|" + std::to_string(n), "http://app/live" + std::to_string(n), t + 50);
      t += 300;
    }
    assert(b.loads.empty() && b.loader.size() == 1);
    // Page 4 stands still and starts loading; then the page turns to 2 before it came.
    b.loader.begin(); b.loader.want(page(4), b.want("live|page4", "live|4", Rank::PAGE, Slot::LIVE)); b.loader.end(t, true);
    assert(b.loads.size() == 1 && b.loads[0] == "2:http://app/live4");
    b.loader.begin(); b.loader.want(page(2), b.want("live|page2", "live|2", Rank::PAGE, Slot::LIVE)); b.loader.end(t += 200, false);
    assert(b.cancels.size() == 1 && !b.loader.busy());
    // The download that was broken off reports nothing; page 2 gets its own.
    b.loader.answer("live|2", "http://app/live2", t + 50);
    b.loader.begin(); b.loader.want(page(2), b.want("live|page2", "live|2", Rank::PAGE, Slot::LIVE)); b.loader.end(t += 900, true);
    assert(b.loads.size() == 2 && b.loads[1] == "2:http://app/live2");
    b.finish(Slot::LIVE, true, t + 300);
    assert(b.store.count("live|page2") && !b.store.count("live|page4"));
  }
  // A card opened and closed before its cover came: the download breaks off, nothing is left behind.
  {
    Bench b;
    b.loader.begin(); b.loader.want(CARD, b.want("cover|p|a|132", "cover|p", Rank::CARD)); b.loader.end(1000, true);
    b.loader.answer("cover|p", "http://app/c", 1100);
    b.loader.begin(); b.loader.want(CARD, b.want("cover|p|a|132", "cover|p", Rank::CARD)); b.loader.end(1200, true);
    assert(b.loader.busy());
    b.loader.begin(); b.loader.end(1300, true);  // the card closed
    assert(b.cancels.size() == 1 && b.loader.size() == 0 && !b.loader.busy());
    b.finish(Slot::FULL, true, 1500);              // a late report of the broken-off download goes nowhere
    assert(b.store.empty() && b.told.empty());
  }
  // Another track: the owner's last cover is forgotten, the new one asked for.
  {
    Bench b;
    b.store["cover|p|a|132"] = 1;
    b.loader.begin(); b.loader.want(CARD, b.want("cover|p|a|132", "cover|p", Rank::CARD)); b.loader.end(1000, true);
    b.loader.begin(); b.loader.want(CARD, b.want("cover|p|b|132", "cover|p", Rank::CARD)); b.loader.end(1250, true);
    assert(b.forgotten.size() == 1 && b.forgotten[0] == "cover|p|a|132" && b.asked.size() == 1);
  }
  // A cover fetched ahead for a kept page goes last: it asks only once nothing on the glass waits, and the page on the
  // glass breaks its download off.
  {
    Bench b;
    auto round = [&](uint32_t t, bool card) {
      b.loader.begin();
      b.loader.want(tile(7), b.want("cover|p|a|196", "cover|p", Rank::AHEAD));
      if (card) b.loader.want(page(2), b.want("live|page2", "live|2", Rank::PAGE, Slot::LIVE));
      b.loader.end(t, true);
    };
    round(1000, false);
    assert(b.asked.size() == 1);
    b.loader.answer("cover|p", "http://app/ahead", 1100);
    round(1200, false);
    assert(b.loads.size() == 1 && b.loads[0] == "0:http://app/ahead");
    round(1300, true);  // the page on the glass wants its strip: the download ahead breaks off
    assert(b.cancels.size() == 1 && b.asked.size() == 2 && b.asked[1] == "live|2");
    b.loader.answer("live|2", "http://app/strip", 1400);
    round(1500, true);
    assert(b.loads.size() == 2 && b.loads[1] == "2:http://app/strip");
    b.finish(Slot::LIVE, true, 1700);
    round(2600, true);  // the glass has its picture: the one ahead goes on, through the link it had
    assert(b.loads.size() == 3 && b.loads[2] == "0:http://app/ahead");
  }
  // One question of a kind at a time: two covers of one player wait for each other's answer.
  {
    Bench b;
    b.loader.begin();
    b.loader.want(CARD, b.want("cover|p|a|132", "cover|p", Rank::CARD));
    b.loader.want(tile(3), b.want("cover|p|a|196", "cover|p", Rank::PAGE));
    b.loader.end(1000, true);
    assert(b.asked.size() == 1);
    b.loader.answer("cover|p", "http://app/132", 1100);
    assert(b.loader.phase(CARD) == Loader::Phase::LINKED && b.loader.phase(tile(3)) == Loader::Phase::NEW);
    b.loader.begin();
    b.loader.want(CARD, b.want("cover|p|a|132", "cover|p", Rank::CARD));
    b.loader.want(tile(3), b.want("cover|p|a|196", "cover|p", Rank::PAGE));
    b.loader.end(1200, true);
    assert(b.asked.size() == 2 && b.loads.size() == 1 && b.loads[0] == "0:http://app/132");
  }
  // A camera refreshes at its pace; a load that fails is tried again after the gap, and a link that fails three times is
  // asked for anew.
  {
    Bench b;
    auto round = [&](uint32_t t) { b.loader.begin(); b.loader.want(VIEW, b.want("camera|c", "full|c", Rank::VIEW, Slot::FULL, 4000)); b.loader.end(t, true); };
    round(1000);
    b.loader.answer("full|c", "http://app/cam", 1100);
    round(1200);
    b.finish(Slot::FULL, true, 1500);
    round(2000); round(5000);
    assert(b.loads.size() == 1);
    round(5600);  // 4 s after the copy was made
    assert(b.loads.size() == 2);
    for (int i = 0; i < 3; ++i) {
      b.finish(Slot::FULL, false, 6000 + i * 2000);
      round(6000 + i * 2000 + 1000);
    }
    assert(b.loader.phase(VIEW) == Loader::Phase::ASKED && b.asked.size() == 2);
    int failed = 0;
    for (auto &t : b.told) failed += t.second == Outcome::FAILED;
    assert(failed == 3);
  }
  // The app has no picture: a cover stays without one; a map (drawn by the app) is asked for again, later each time.
  {
    Bench b;
    auto round = [&](uint32_t t) {
      b.loader.begin();
      b.loader.want(CARD, b.want("cover|radio", "cover|radio", Rank::CARD));
      b.loader.want(page(4), b.want("live|map", "live|map", Rank::PAGE, Slot::LIVE, 0, true));
      b.loader.end(t, true);
    };
    round(1000);
    b.loader.answer("cover|radio", "", 1100);
    b.loader.answer("live|map", "", 1100);
    assert(b.told.size() == 2 && b.told[0].second == Outcome::NONE && b.loader.settled_for(CARD));
    for (uint32_t t = 1250; t < 12000; t += 250) round(t);
    assert(b.asked.size() == 3);  // the map once more after ten seconds, the cover never
    b.loader.answer("live|map", "", 11200);
    for (uint32_t t = 11250; t < 31000; t += 250) round(t);
    assert(b.asked.size() == 3);  // twenty seconds the second time
    round(31250);
    assert(b.asked.size() == 4);
  }
  // An answer that never came is asked for again; nothing loads while a finger is down or the pages move (may_load).
  {
    Bench b;
    auto round = [&](uint32_t t, bool may) { b.loader.begin(); b.loader.want(ALERT, b.want("alert|c", "alert|c", Rank::ALERT, Slot::THUMB)); b.loader.end(t, may); };
    round(1000, true);
    round(10900, true);
    assert(b.asked.size() == 1);
    round(11000, true);
    assert(b.asked.size() == 2);
    b.loader.answer("alert|c", "http://app/a", 11100);
    round(11200, false);
    assert(b.loads.empty());
    round(11450, true);
    assert(b.loads.size() == 1 && b.loads[0] == "1:http://app/a");
  }
  // The most urgent picture goes first: an alert before a card before a page.
  {
    Bench b;
    b.loader.begin();
    b.loader.want(page(1), b.want("live|1", "live|1", Rank::PAGE, Slot::LIVE));
    b.loader.want(CARD, b.want("cover|p", "cover|p", Rank::CARD));
    b.loader.want(ALERT, b.want("alert|c", "alert|c", Rank::ALERT, Slot::THUMB));
    b.loader.end(1000, true);
    for (auto tag : {"live|1", "cover|p", "alert|c"}) b.loader.answer(tag, std::string("http://app/") + tag, 1100);
    b.loader.begin();
    b.loader.want(page(1), b.want("live|1", "live|1", Rank::PAGE, Slot::LIVE));
    b.loader.want(CARD, b.want("cover|p", "cover|p", Rank::CARD));
    b.loader.want(ALERT, b.want("alert|c", "alert|c", Rank::ALERT, Slot::THUMB));
    b.loader.end(1200, true);
    assert(b.loads.size() == 1 && b.loads[0] == "1:http://app/alert|c");
    // A download on the glass is not broken off for another on the glass: the card waits for the alert's.
    assert(b.cancels.empty());
  }
  // An owner that is done for good (the camera full screen closed): its picture and download go.
  {
    Bench b;
    b.loader.begin(); b.loader.want(VIEW, b.want("camera|c", "full|c", Rank::VIEW, Slot::FULL, 4000)); b.loader.end(1000, true);
    b.loader.answer("full|c", "http://app/cam", 1100);
    b.loader.begin(); b.loader.want(VIEW, b.want("camera|c", "full|c", Rank::VIEW, Slot::FULL, 4000)); b.loader.end(1200, true);
    b.loader.release(VIEW);
    assert(b.cancels.size() == 1 && b.forgotten.size() == 1 && b.loader.size() == 0);
  }
  // A picture the store holds is told to its owner once, as a download would be; a link the owner has is loaded without
  // a question; a picture with a number of tries is given up after them.
  {
    Bench b;
    b.store["cover|kept"] = 1;
    for (uint32_t t = 1000; t < 2000; t += 250) { b.loader.begin(); b.loader.want(CARD, b.want("cover|kept", "cover|p", Rank::CARD)); b.loader.end(t, true); }
    assert(b.told.size() == 1 && b.told[0].second == Outcome::UNCHANGED);
    Bench a;
    auto alert = [&](uint32_t t) {
      Want w = a.want("alert|c|1", "alert|c", Rank::ALERT, Slot::THUMB);
      w.url = "http://app/alert"; w.tries = 3; w.ask = nullptr;
      a.loader.begin(); a.loader.want(ALERT, w); a.loader.end(t, true);
    };
    alert(1000);
    assert(a.asked.empty() && a.loads.size() == 1 && a.loads[0] == "1:http://app/alert");
    for (int i = 0; i < 3; ++i) { a.finish(Slot::THUMB, false, 1200 + i * 1000); alert(1200 + i * 1000 + 900); }
    assert(a.loads.size() == 3 && a.loader.phase(ALERT) == Loader::Phase::NONE);
    for (uint32_t t = 5000; t < 60000; t += 250) alert(t);
    assert(a.loads.size() == 3 && a.asked.empty());
  }
  // An owner that wanted nothing for a while and comes back with another picture (a card opened again on the next track)
  // lets the one it had go; back with the same picture, it keeps it.
  {
    Bench b;
    b.store["cover|p|a"] = 1;
    b.loader.begin(); b.loader.want(CARD, b.want("cover|p|a", "cover|p", Rank::CARD)); b.loader.end(1000, true);
    b.loader.begin(); b.loader.end(1250, true);  // the card closed
    b.loader.begin(); b.loader.want(CARD, b.want("cover|p|a", "cover|p", Rank::CARD)); b.loader.end(1500, true);
    assert(b.forgotten.empty());
    b.loader.begin(); b.loader.end(1750, true);
    b.loader.begin(); b.loader.want(CARD, b.want("cover|p|b", "cover|p", Rank::CARD)); b.loader.end(2000, true);
    assert(b.forgotten.size() == 1 && b.forgotten[0] == "cover|p|a");
  }
  printf("test_picture_loader: ok\n");
  return 0;
}
