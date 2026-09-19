#include "screen_text_en.h"
// The screen's texts (app 0.2.90): lookups, placeholders, plural forms and numbers, with the English table the tests
// build with (tests/screen_text_en.h). A screen's own build writes its language's table instead.
#include <cassert>
#include <cstdio>
#include <string>
using namespace screen_text;

int main() {
  assert(std::string(LANGUAGE) == "en");
  assert(std::string(tr(txt::settings_title)) == "Settings");
  assert(std::string(tr(KEY_COUNT)) == "" && std::string(tr(0xFFFF)) == "");
  // A list is its first key plus a count: the month names in order.
  assert(txt::date_months_short_count == 12 && std::string(tr(txt::date_months_short + 8)) == "Sep");
  assert(txt::date_weekdays_count == 7 && std::string(tr(txt::date_weekdays)) == "Sunday");

  // Named placeholders, every occurrence; a translation is never a printf format.
  assert(fill(std::string("{n} of {n}"), "n", "3") == "3 of 3");
  assert(fill(txt::tile_page, "n", 2) == "Page 2");
  assert(fill(std::string("100%"), "n", "1") == "100%");

  // Plural forms by the language's rule (English: one for 1, other for the rest); the one form may name the number.
  assert(plural(txt::time_hours_ago, 1) == "1 hour ago" && plural(txt::time_hours_ago, 2) == "2 hours ago");
  assert(plural(txt::time_hours_ago, 0) == "0 hours ago" && plural(txt::history_times, 1) == "once");
  assert(plural(txt::history_times, 21) == "21 times");
  // A text without forms is the same for every n.
  assert(plural(txt::time_minutes_ago, 5) == "5 min ago");

  // Numbers: the language's marks unless Settings -> Language & region chose a format for every screen.
  number_style = 0;
  assert(localize("1234.5") == "1,234.5" && localize("-3") == "-3" && localize("123") == "123");
  assert(localize("1234567") == "1,234,567" && localize("0.25") == "0.25");
  for (const char *text : {"on", "1.2.3", "-", ".5", "5.", "12a", "", "1e5"}) assert(localize(text) == text);
  assert(decimal(21.54f, 1) == "21.5");
  number_style = 2;  // 1.234,5
  assert(localize("1234.5") == "1.234,5" && localize("-1234") == "-1.234" && decimal(21.54f, 1) == "21,5");
  number_style = 3;  // 1 234,5
  assert(localize("12345.75") == "12 345,75");
  number_style = 1;  // 1,234.5
  assert(localize("1234.5") == "1,234.5");
  assert(number_style_of("auto") == 0 && number_style_of("point") == 1 && number_style_of("comma") == 2 &&
         number_style_of("space") == 3 && number_style_of("dot") == -1);
  number_style = 0;
  std::puts("test_screen_text: PASS");
  return 0;
}
