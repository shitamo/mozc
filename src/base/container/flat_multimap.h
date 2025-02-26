// Copyright 2010-2021, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef MOZC_BASE_CONTAINER_FLAT_MULTIMAP_H_
#define MOZC_BASE_CONTAINER_FLAT_MULTIMAP_H_

#include <stddef.h>

#include <algorithm>
#include <array>
#include <functional>
#include <utility>

#include "absl/types/span.h"
#include "base/container/entry.h"
#include "base/container/flat_internal.h"

namespace mozc {

// Read-only multimap that is backed by `constexpr`-sorted array.
template <class K, class V, class CompareKey, size_t N>
class FlatMultimap {
 public:
  // Consider calling `CreateFlatMultimap` instead, so you don't have to
  // manually specify the number of entries, `N`.
  constexpr FlatMultimap(std::array<Entry<K, V>, N> entries,
                         const CompareKey &cmp_key = {})
      : entries_(std::move(entries)), cmp_key_(cmp_key) {
    internal::Sort(absl::MakeSpan(entries_),
                   [&](const Entry<K, V> &a, const Entry<K, V> &b) {
                     return cmp_key_(a.key, b.key);
                   });
  }

  // Returns a span of entries with the given key.
  //
  // IMPORTANT: The order of the returned span is not guaranteed to be the
  // same as the order of the entries given when the map was created.
  //
  // NOTE: The current implementation actually preserves the order, but this
  // will change once we switch to `std::sort`, which is not stable (and
  // `std::stable_sort` is not planned to receive constexpr support).
  constexpr absl::Span<const Entry<K, V>> EqualSpan(const K &key) const {
    auto lb = internal::FindFirst(
        absl::MakeSpan(entries_),
        [&](const Entry<K, V> &e) { return !cmp_key_(e.key, key); });
    auto ub = internal::FindFirst(
        absl::MakeSpan(entries_),
        [&](const Entry<K, V> &e) { return cmp_key_(key, e.key); });
    return absl::MakeConstSpan(lb, ub);
  }

 private:
  std::array<Entry<K, V>, N> entries_;
  CompareKey cmp_key_;
};

// Creates a `FlatMultimap` from a C-style array of `Entry`s.
//
// Example:
//
//   constexpr auto kMultimap = CreateFlatMultimap<int, absl::string_view>({
//       {1, "one"},
//       {1, "ichi"},
//       {2, "two"},
//       {2, "ni"},
//       {3, "three"},
//       {3, "san"},
//   });
// Declare the variable as auto and use `CreateFlatMultimap`. The actual type is
// complex and explicitly declaring it would leak the number of entries, `N`.
template <class K, class V, class CompareKey = std::less<>, size_t N>
constexpr auto CreateFlatMultimap(Entry<K, V> (&&entries)[N],
                                  const CompareKey &cmp_key = CompareKey()) {
  return FlatMultimap<K, V, CompareKey, N>(
      internal::ToArray(std::move(entries)), cmp_key);
}

}  // namespace mozc

#endif  // MOZC_BASE_CONTAINER_FLAT_MULTIMAP_H_
