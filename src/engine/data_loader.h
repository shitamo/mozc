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

#ifndef MOZC_ENGINE_DATA_LOADER_H_
#define MOZC_ENGINE_DATA_LOADER_H_

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "base/thread.h"
#include "engine/modules.h"
#include "protocol/engine_builder.pb.h"

namespace mozc {

// DataLoader receives requests for loading language model data and loads the
// data from the top priority request. The language model data is asynchronously
// loaded in a sub-thread.
// DataLoader is designed to be thread-compatible, similar to other components
// like std::vector and most of other Mozc classes.
// https://blog.reverberate.org/2021/12/18/thread-safety-cpp-rust.html
class DataLoader {
 public:
  DataLoader() = default;
  DataLoader(const DataLoader &) = delete;
  DataLoader &operator=(const DataLoader &) = delete;
  virtual ~DataLoader() = default;

  struct Response {
    uint64_t id = 0;  // engine id. Fingerprint of EngineReloadRequest.
    EngineReloadResponse response;
    std::unique_ptr<engine::Modules> modules;
  };

  // Wrapped with BackgroundFuture so the data loading is
  // executed asynchronously.
  using ResponseFuture = BackgroundFuture<Response>;

  // Returns the request id associated with the request.
  uint64_t GetRequestId(const EngineReloadRequest &request) const;

  // Accepts engine reload request and immediately returns the engine id with
  // the highest priority defined as follows:
  //  - Request with higher request priority (e.g., downloaded > bundled)
  //  - When the priority is the same, the request registered last.
  // The engine id 0 is reserved for unused engine.
  uint64_t RegisterRequest(const EngineReloadRequest &request);

  // Unregister the request associated with the `id` and immediately returns
  // the new engine id after the unregistration. This method is usually called
  // to notify the request is not processed due to model loading failures and
  // avoid the multiple loading operations.  Client needs to load or use the
  // engine of returned id. The unregistered request will not be accepted after
  // calling this method.
  uint64_t ReportLoadFailure(uint64_t id);

  // Sets the id of DataLoader::Response as the ID of the currently using data.
  void ReportLoadSuccess(uint64_t id) { current_request_id_ = id; }

  // Builds the new engine associated with `id`.
  // This method returns the future object immediately.
  // All errors are stored in EngineReloadResponse::response::status.
  virtual ResponseFuture Build(uint64_t id) const;

  void Clear();

  // Maybe build a new data from the top priority request if a reload request
  // has been already received and no existing build process is running.
  void MaybeBuildNewData();

  // Returns true if a new data loader response is ready.
  bool IsBuildResponseReady() const;

  // Maybe move the data loader response to the caller.
  // Otherwise nullptr is returned.
  std::unique_ptr<DataLoader::Response> MaybeMoveDataLoaderResponse();

  // Used only in unittest to perform blocking behavior.
  void SetAlwaysWaitForLoaderResponseFutureForTesting(bool value) {
    always_wait_for_loader_response_future_ = value;
  }

 private:
  struct RequestData {
    uint64_t id = 0;           // Fingerprint of request.
    int32_t priority = 0;      // Priority of the model. smaller is better.
    uint32_t sequence_id = 0;  // Sequential id.
    EngineReloadRequest request;
  };

  // Sequential counter assigned to RequestData.
  uint32_t sequence_id_ = 0;

  absl::flat_hash_set<uint64_t> unregistered_;
  std::vector<RequestData> requests_;

  // Id of the highest priority request in the registered requests.
  // 0 means that no request have been registered or valid yet.
  uint64_t top_request_id_ = 0;

  // Id of the request for the current data.
  // 0 means that no data has been updated yet.
  uint64_t current_request_id_ = 0;

  std::optional<DataLoader::ResponseFuture> loader_response_future_;
  // used only in unittest to perform blocking behavior.
  bool always_wait_for_loader_response_future_ = false;
};

}  // namespace mozc

#endif  // MOZC_ENGINE_DATA_LOADER_H_
