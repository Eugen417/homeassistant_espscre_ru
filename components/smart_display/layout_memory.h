#pragma once
#include <vector>
#if __has_include("esphome/core/defines.h")
#include "esphome/core/defines.h"
#endif
#ifdef USE_ESP32
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include <cstdlib>
#endif

// Page and tile records share the existing PSRAM-first allocation policy.
// Model::begin checks space before allocating. A final allocation failure must
// stop safely instead of letting a vector write through a null pointer.
namespace layout_memory {
#ifdef USE_ESP32
template<class T> struct Allocator {
  using value_type = T;
  Allocator() = default;
  template<class U> Allocator(const Allocator<U> &) {}
  T *allocate(size_t n) {
    T *p = esphome::RAMAllocator<T>().allocate(n);
    if (!p) {
      ESP_LOGE("runtime", "No memory for %u layout records (%u bytes)", static_cast<unsigned>(n), static_cast<unsigned>(n * sizeof(T)));
      abort();
    }
    return p;
  }
  void deallocate(T *p, size_t n) { esphome::RAMAllocator<T>().deallocate(p, n); }
  bool operator==(const Allocator &) const { return true; }
  bool operator!=(const Allocator &) const { return false; }
};
template<class T> using Vector = std::vector<T, Allocator<T>>;
#else
template<class T> using Vector = std::vector<T>;
#endif
}  // namespace layout_memory
