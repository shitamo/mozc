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

#ifndef MOZC_ENGINE_SUPPLEMENTAL_MODEL_INTERFACE_H_
#define MOZC_ENGINE_SUPPLEMENTAL_MODEL_INTERFACE_H_

#include <optional>
#include <vector>

#include "absl/strings/string_view.h"
#include "composer/query.h"
#include "converter/segments.h"
#include "protocol/commands.pb.h"
#include "protocol/engine_builder.pb.h"

namespace mozc::engine {

// Abstract interface of supplemental model.
class SupplementalModelInterface {
 public:
  virtual ~SupplementalModelInterface() = default;

  // Loads supplemental model asynchronously defined in the `request`.
  // Returns false if the LoadAsync is already running.
  virtual bool LoadAsync(const EngineReloadRequest &request) { return false; }

  // Loads supplemental model defined in the `request`.
  virtual EngineReloadResponse Load(const EngineReloadRequest &request) {
    return EngineReloadResponse();
  }

  // Performs spelling correction.
  // `request.text` may contains multiple sentences.
  virtual std::optional<commands::CheckSpellingResponse> CheckSpelling(
      const commands::CheckSpellingRequest &request) const {
    return std::nullopt;
  }

  // Performs spelling correction for composition (pre-edit) Hiragana sequence.
  // Both `query` and `context` must be Hiragana input sequence.
  // `request` is passed to determine the keyboard layout.
  // Returns empty result when no correction is required.
  // Returns std::nullopt when the composition spellchecker is not
  // enabled/available.
  virtual std::optional<std::vector<composer::TypeCorrectedQuery>>
  CorrectComposition(absl::string_view query, absl::string_view context,
                     bool disable_toggle_correction,
                     const commands::Request &request) const {
    return std::nullopt;
  }

  // Performs general post correction on `segments`.
  virtual void PostCorrect(Segments *segments) const {}
};

}  // namespace mozc::engine

#endif  // MOZC_ENGINE_SUPPLEMENTAL_MODEL_INTERFACE_H_
