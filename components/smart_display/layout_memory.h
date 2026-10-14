#pragma once
#include <cstddef>
#include <limits>
#include <new>
#include <utility>
#if __has_include("esphome/core/defines.h")
#include "esphome/core/defines.h"
#endif
#ifdef USE_ESP32
#include "esphome/core/helpers.h"
#endif

namespace layout_memory {
// ESP32 builds do not have exceptions: std::vector cannot report allocation
// failure safely. Reservations hold raw storage only, never a second layout.
#ifndef USE_ESP32
inline bool (*allocation_allowed)(size_t) = nullptr;  // Host fault injection.
#endif
template<class T> class Vector {
  T *data_ = nullptr;
  size_t size_ = 0, capacity_ = 0;
  static T *allocate(size_t n) {
    if (!n || n > std::numeric_limits<size_t>::max() / sizeof(T)) return nullptr;
#ifndef USE_ESP32
    if (allocation_allowed && !allocation_allowed(n * sizeof(T))) return nullptr;
    return static_cast<T *>(::operator new(n * sizeof(T), std::nothrow));
#else
    return esphome::RAMAllocator<T>().allocate(n);
#endif
  }
  static void release(T *data, size_t capacity) {
    if (!data) return;
#ifdef USE_ESP32
    esphome::RAMAllocator<T>().deallocate(data, capacity);
#else
    (void) capacity;
    ::operator delete(data);
#endif
  }
 public:
  class Reservation {
    friend class Vector;
    T *data_;
    size_t count_;
    bool valid_;
    Reservation(size_t count, size_t capacity)
        : data_(count > capacity ? allocate(count) : nullptr), count_(count),
          valid_(count <= capacity || data_ != nullptr) {}
   public:
    Reservation(const Reservation &) = delete;
    Reservation &operator=(const Reservation &) = delete;
    ~Reservation() { release(data_, count_); }
    explicit operator bool() const { return valid_; }
  };
  Vector() = default;
  explicit Vector(size_t count) { resize(count); }
  Vector(const Vector &) = delete;
  Vector &operator=(const Vector &) = delete;
  ~Vector() { clear(); release(data_, capacity_); }
  size_t size() const { return size_; }
  size_t capacity() const { return capacity_; }
  bool empty() const { return size_ == 0; }
  T &operator[](size_t index) { return data_[index]; }
  const T &operator[](size_t index) const { return data_[index]; }
  T *begin() { return data_; }
  T *end() { return size_ ? data_ + size_ : data_; }
  const T *begin() const { return data_; }
  const T *end() const { return size_ ? data_ + size_ : data_; }
  void pop_back() { if (size_) data_[--size_].~T(); }
  void clear() { while (size_) pop_back(); }
  Reservation prepare(size_t count) const { return Reservation(count, capacity_); }
  // Call after all participating buffers have successful reservations. Equal
  // or smaller replacements reuse capacity without a second tile allocation.
  void reset(Reservation &reservation) {
    clear();
    if (reservation.data_) {
      release(data_, capacity_);
      data_ = reservation.data_; capacity_ = reservation.count_;
      reservation.data_ = nullptr;
    }
    while (size_ < reservation.count_) new (data_ + size_++) T();
  }
  bool resize(size_t count) {
    auto reservation = prepare(count);
    if (!reservation) return false;
    if (reservation.data_) {
      for (size_t i = 0; i < size_; ++i) new (reservation.data_ + i) T(std::move(data_[i]));
      const auto previous = size_;
      clear(); release(data_, capacity_);
      data_ = reservation.data_; capacity_ = count; size_ = previous;
      reservation.data_ = nullptr;
    }
    while (size_ > count) pop_back();
    while (size_ < count) new (data_ + size_++) T();
    return true;
  }
  bool push_back(const T &value) {
    if (!resize(size_ + 1)) return false;
    data_[size_ - 1] = value;
    return true;
  }
};
}  // namespace layout_memory
