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


#ifdef _WIN32
#include <crtdbg.h>
#endif  // _WIN32

#include "absl/flags/flag.h"
#include "base/init_mozc.h"
#include "testing/gunit.h"

int main(int argc, char **argv) {
  // TODO(yukawa, team): Implement b/2805528 so that you can specify any option
  // given by gunit.
#ifdef GTEST_HAS_ABSL
  // Skip passing a usage if GoogleTest is built with Abseil, GoogleTest will
  // provide its own usage in this configuration.
  const char* usage = nullptr;
#else   // !GTEST_HAS_ABSL
  const char* usage = argv[0];
#endif  // GTEST_HAS_ABSL
  mozc::InitMozc(usage, &argc, &argv);
  testing::InitGoogleTest(&argc, argv);

#ifdef _WIN32
  _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
  _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#endif  // _WIN32

  // Without this flag, ::RaiseException makes the job stuck.
  // See b/2805521 for details.
  GTEST_FLAG_SET(catch_exceptions, true);

  return RUN_ALL_TESTS();
}

