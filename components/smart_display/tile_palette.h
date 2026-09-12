#pragma once
#include <cstdint>
#include <string>

namespace tile_palette {
// Match the editor palette; all backgrounds are chosen for dark text.
inline uint32_t color(const std::string &name) {
  if(name=="red")return 0xFADADD;
  if(name=="orange")return 0xFFE1C6;
  if(name=="yellow")return 0xFFF0C2;
  if(name=="green")return 0xD9EEDC;
  if(name=="mint")return 0xD5F0EA;
  if(name=="blue")return 0xD9EAFB;
  if(name=="purple")return 0xE9DDF5;
  if(name=="pink")return 0xF7DDEC;
  if(name=="gray")return 0xE5E7EB;
  return 0;  // Automatic/default, including unknown future palette names.
}
}
