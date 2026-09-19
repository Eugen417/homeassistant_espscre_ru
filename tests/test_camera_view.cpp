#include "screen_text_en.h"
// clang++ -std=c++17 -Wall -Wextra -Werror -I. tests/test_camera_view.cpp -o /tmp/test_camera_view && /tmp/test_camera_view
#include "../components/smart_display/camera_view.h"
#include <cassert>

using namespace camera_view;

int main() {
  Feed feed;
  assert(!feed.open() && !feed.should_ask(0) && !feed.should_load(0));

  // Opening asks at once; a link that does not come is asked for again after ASK_AGAIN_MS.
  feed.open("camera.front_door");
  assert(feed.should_ask(1000));
  feed.ask(1000);
  assert(!feed.should_ask(1000 + ASK_AGAIN_MS - 1));
  assert(feed.should_ask(1000 + ASK_AGAIN_MS));

  // A link loads at once, then every REFRESH_MS from the start of the last load.
  feed.link("http://192.168.1.2:8098/camera/abcdefghijklmnopqrstuvwx.jpg");
  assert(!feed.should_ask(2000) && feed.should_load(2000));
  feed.start(2000);
  assert(!feed.should_load(2100) && !feed.should_ask(2100));
  feed.finish(2300, true);
  assert(feed.shown && !feed.should_load(2000 + REFRESH_MS - 1) && feed.should_load(2000 + REFRESH_MS));

  // A load that ends near the next start still leaves GAP_MS for touch and drawing before the next one.
  const uint32_t slow_end = 5000 + REFRESH_MS - 100;
  feed.start(5000);
  feed.finish(slow_end, true);
  assert(!feed.should_load(5000 + REFRESH_MS) && !feed.should_load(slow_end + GAP_MS - 1) && feed.should_load(slow_end + GAP_MS));
  // An ordinary load keeps the rhythm: the next start is REFRESH_MS after this one, however quickly it ended.
  feed.start(20000);
  feed.finish(20000 + REFRESH_MS / 2, true);
  assert(!feed.should_load(20000 + REFRESH_MS - 1) && feed.should_load(20000 + REFRESH_MS));

  // Failures keep the link until MAX_FAILURES in a row; then a new link is asked for, the image stays shown.
  uint32_t now = 10000;
  for (uint8_t i = 0; i < MAX_FAILURES; ++i) {
    assert(!feed.url.empty());
    feed.start(now);
    feed.finish(now + 100, false);
    now += REFRESH_MS;
  }
  assert(feed.url.empty() && feed.shown && feed.should_ask(now));

  // An app without an image answers an empty link: nothing loads, and the view asks again later.
  feed.ask(now);
  feed.link("");
  assert(feed.empty && !feed.should_load(now) && !feed.should_ask(now + 1) && feed.should_ask(now + ASK_AGAIN_MS));

  // A success after a failure starts the count again.
  feed.link("http://192.168.1.2:8098/camera/abcdefghijklmnopqrstuvwx.jpg");
  feed.start(now); feed.finish(now + 10, false);
  feed.start(now + REFRESH_MS); feed.finish(now + REFRESH_MS + 10, true);
  assert(feed.failures == 0 && !feed.url.empty());

  // Opening another camera forgets everything of the last one; millis() 0 still counts as a start.
  feed.open("image.doorbell_snapshot");
  assert(feed.url.empty() && !feed.shown && feed.should_ask(0));
  feed.link("http://h/camera/x.jpg");
  feed.start(0);
  feed.finish(10, true);
  assert(!feed.should_load(REFRESH_MS - 1) && feed.should_load(REFRESH_MS + 1));

  // An album cover (firmware 0.2.64+) loads once per link: it stays on screen, never refreshed on a clock.
  Feed cover;
  cover.open("media_player.office", true);
  assert(cover.once && cover.should_ask(0));
  cover.ask(0);
  cover.link("http://h/camera/cover.bmp");
  assert(cover.should_load(100));
  cover.start(100);
  cover.finish(400, true);
  assert(cover.loaded && cover.shown && !cover.should_load(400 + GAP_MS) && !cover.should_load(400 + 10 * REFRESH_MS) && !cover.should_ask(400 + ASK_AGAIN_MS));
  // A failed load is tried again after the gap; three failures forget the link so the card asks for a new one.
  cover.link("http://h/camera/cover2.bmp");
  assert(!cover.loaded && cover.should_load(1000));
  cover.start(1000); cover.finish(1100, false);
  assert(!cover.should_load(1100 + GAP_MS - 1) && cover.should_load(1100 + GAP_MS));
  cover.start(2000); cover.finish(2100, false);
  cover.start(3000); cover.finish(3100, false);
  assert(cover.url.empty() && cover.should_ask(3100));
  // A new link (a new picture in Home Assistant) loads again, once.
  cover.ask(3100);
  cover.link("http://h/camera/cover3.bmp");
  cover.start(3200); cover.finish(3500, true);
  assert(cover.loaded && !cover.should_load(3500 + 10 * REFRESH_MS));
  // A player without a picture (or an app from before covers) answers an empty link: asked once, then left alone.
  Feed bare;
  bare.open("media_player.radio", true);
  bare.ask(0);
  bare.link("");
  assert(bare.empty && !bare.should_ask(ASK_AGAIN_MS) && !bare.should_ask(100 * ASK_AGAIN_MS) && !bare.should_load(ASK_AGAIN_MS));
  // A camera feed keeps its clock: open() without the flag.
  Feed live;
  live.open("camera.front_door");
  assert(!live.once);
  return 0;
}
