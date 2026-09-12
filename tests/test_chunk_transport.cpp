#include "../components/smart_display/chunk_transport.h"
#include <cassert>
int main() {
  runtime_tiles::Chunks c;
  assert(c.accept("a|0|0|YWJj", 100)); assert(!c.complete);
  assert(c.accept("a|1|1|ZA==", 101)); assert(c.complete && c.data == "YWJjZA==");
  assert(!c.accept("a|1|1|ZA==", 102)); // duplicate cannot reapply a command
  assert(c.accept("b|0|0|YWJj", 200));
  assert(!c.accept("b|2|1|ZA==", 201)); // dropped chunk
  assert(c.accept("c|0|0|YWJj", 300));
  assert(!c.accept("other|1|1|ZA==", 301)); // another automation interleaved
  assert(c.accept("d|0|0|YWJj", 400));
  assert(!c.accept("d|1|1|ZA==", 10401)); // expired transfer
  assert(!c.accept("bad", 20000));
  assert(!c.accept("x|-1|1|YWJj", 20000));
  assert(!c.accept("x|0|1|not base64", 20000));
  assert(!c.accept("x|0|1|" + std::string(201, 'A'), 20000));
  assert(c.accept("e|0|0|YWJj", 20000));
  assert(c.accept("f|0|1|ZA==", 20001)); assert(c.complete && c.data == "ZA==");
}
