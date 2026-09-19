#pragma once
// The texts a screen shows, in the one language its firmware was built for (app 0.2.90). Every text has a key in the
// `screen` section of screen_manager/translations/<code>.json; screen_text_keys.h numbers the keys, and the build
// writes that language's TABLE into main.cpp (components/smart_display/screen_text_gen.py). A lookup is an index into
// flash: no text is copied, searched for or kept in RAM, and a screen carries no language but its own.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include "screen_text_keys.h"

namespace screen_text {

extern const char *const TABLE[];     // this build's language, one text per key
extern const char *const LANGUAGE;    // its code, such as "nl"; the "Screen language" sensor reports it
int plural_index(int n);              // which form of a plural text fits n in this language

inline const char *tr(uint16_t id) { return id < KEY_COUNT ? TABLE[id] : ""; }

// `text` with every {name} replaced by `value`. Placeholders are named, so a language puts them where its sentence
// needs them; a translation is never used as a printf format.
inline std::string fill(const std::string &text, const char *name, const std::string &value) {
  std::string out = text, mark = std::string("{") + name + "}";
  for (size_t at = out.find(mark); at != std::string::npos; at = out.find(mark, at + value.size()))
    out.replace(at, mark.size(), value);
  return out;
}
inline std::string fill(uint16_t id, const char *name, const std::string &value) { return fill(std::string(tr(id)), name, value); }
inline std::string fill(uint16_t id, const char *name, int value) { return fill(id, name, std::to_string(value)); }

// The form of a plural text ("1 hour ago | {n} hours ago") for n, with {n} filled in. The forms stand in the order
// of the language's plural rule; a text with fewer forms than the rule uses its last one.
inline std::string plural(uint16_t id, int n) {
  const char *text = tr(id);
  int wanted = plural_index(n);
  const char *start = text;
  for (int form = 0; form < wanted; ++form) {
    const char *bar = strchr(start, '|');
    if (!bar) break;
    start = bar + 1;
  }
  const char *end = strchr(start, '|');
  std::string form = end ? std::string(start, end - start) : std::string(start);
  size_t first = form.find_first_not_of(' '), last = form.find_last_not_of(' ');
  form = first == std::string::npos ? std::string() : form.substr(first, last - first + 1);
  return fill(form, "n", std::to_string(n));
}

// How numbers are written (app 0.2.90): ESP Screens sends the choice of Settings -> Language & region, the one place
// for every screen. 0 follows the language (screen.number), 1 is "1,234.5", 2 "1.234,5" and 3 "1 234,5".
inline uint8_t number_style = 0;
inline char decimal_mark() {
  if (number_style == 1) return '.';
  if (number_style == 2 || number_style == 3) return ',';
  const char *mark = tr(txt::number_decimal);
  return mark[0] ? mark[0] : '.';
}
// The separator between thousands.
inline const char *group_mark() {
  if (number_style == 1) return ",";
  if (number_style == 2) return ".";
  if (number_style == 3) return " ";
  return tr(txt::number_group);
}
// How many digits a whole number needs before it gets separators: 4 ("1,234") in most languages, 5 in Italian, Spanish
// and Polish, which write 1234 but 12.345 (CLDR's minimum grouping digits, as Home Assistant's own numbers do).
inline size_t group_from() {
  if (number_style) return 4;
  int minimum = atoi(tr(txt::number_group_min));
  return minimum >= 2 ? 5 : 4;
}
// A number the way this screen writes numbers: `whole` digits (no sign) and the digits after the point.
inline std::string write_number(const std::string &whole, const std::string &fraction) {
  std::string out = whole;
  if (out.size() >= group_from()) {
    const std::string group = group_mark();
    for (int i = static_cast<int>(out.size()) - 3; i > 0; i -= 3) out.insert(static_cast<size_t>(i), group);
  }
  if (!fraction.empty()) out += std::string(1, decimal_mark()) + fraction;
  return out;
}
// A state as Home Assistant sends a number ("1234.5", "-3") written the way this screen writes numbers ("1.234,5" in
// Dutch); any other text as it is.
inline std::string localize(const std::string &state) {
  size_t start = !state.empty() && state[0] == '-' ? 1 : 0, dot = std::string::npos;
  if (start == state.size()) return state;
  for (size_t i = start; i < state.size(); ++i) {
    if (state[i] == '.' && dot == std::string::npos && i > start && i + 1 < state.size()) dot = i;
    else if (state[i] < '0' || state[i] > '9') return state;
  }
  std::string whole = state.substr(start, dot == std::string::npos ? std::string::npos : dot - start);
  std::string fraction = dot == std::string::npos ? std::string() : state.substr(dot + 1);
  return state.substr(0, start) + write_number(whole, fraction);
}
// "point", "comma", "space" or "auto" as its number_style; -1 for anything else.
inline int number_style_of(const std::string &name) {
  if (name == "auto") return 0;
  if (name == "point") return 1;
  if (name == "comma") return 2;
  if (name == "space") return 3;
  return -1;
}
// A number with `decimals` places, written the way this language writes it ("21,5" in Dutch).
inline std::string decimal(float value, int decimals) {
  char buffer[24];
  snprintf(buffer, sizeof(buffer), "%.*f", decimals, (double) value);
  std::string text = buffer;
  if (decimal_mark() != '.') {
    size_t dot = text.find('.');
    if (dot != std::string::npos) text[dot] = decimal_mark();
  }
  return text;
}

}  // namespace screen_text
