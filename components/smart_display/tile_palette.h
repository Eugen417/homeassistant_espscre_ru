#pragma once
#include <cstdint>
#include <string>
#include "theme.h"

namespace tile_palette {
// The named card colours ESP Screens offers (theme::SWATCHES). A tile and an alert keep the light value as the colour's
// identity; theme::surface() draws it in the current look. Unknown future names and "auto" are 0: the normal card.
inline uint32_t color(const std::string &name) { return theme::swatch(name); }
// "None": no card behind the tile contents. Older firmware treats it as automatic.
inline bool transparent(const std::string &name) { return name=="none"; }
}
