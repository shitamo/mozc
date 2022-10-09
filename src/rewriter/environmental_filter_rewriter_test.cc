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

#include "rewriter/environmental_filter_rewriter.h"

#include <memory>
#include <string>
#include <vector>

#include "base/system_util.h"
#include "base/util.h"
#include "converter/segments.h"
#include "data_manager/emoji_data.h"
#include "data_manager/testing/mock_data_manager.h"
#include "protocol/commands.pb.h"
#include "request/conversion_request.h"
#include "testing/base/public/googletest.h"
#include "testing/base/public/gunit.h"
#include "absl/container/btree_map.h"
#include "absl/flags/flag.h"
#include "absl/strings/string_view.h"

namespace mozc {
namespace {
constexpr char kKanaSupplement_6_0[] = "\U0001B001";
constexpr char kKanaSupplement_10_0[] = "\U0001B002";
constexpr char kKanaExtendedA_14_0[] = "\U0001B122";

void AddSegment(const std::string &key, const std::string &value,
                Segments *segments) {
  segments->Clear();
  Segment *seg = segments->push_back_segment();
  seg->set_key(key);
  Segment::Candidate *candidate = seg->add_candidate();
  candidate->Init();
  candidate->value = value;
  candidate->content_value = value;
}

void AddSegment(const std::string &key, const std::vector<std::string> &values,
                Segments *segments) {
  Segment *seg = segments->add_segment();
  seg->set_key(key);
  for (const std::string &value : values) {
    Segment::Candidate *candidate = seg->add_candidate();
    candidate->content_key = key;
    candidate->value = value;
    candidate->content_value = value;
  }
}

struct EmojiData {
  const absl::string_view emoji;
  const EmojiVersion unicode_version;
};

// Elements must be sorted lexicographically by key (first string).
constexpr EmojiData kTestEmojiList[] = {
    // Emoji 12.1 Example
    {"🧑‍✈", EmojiVersion::E12_1},   // 1F9D1 200D 2708
    {"🧑‍⚖", EmojiVersion::E12_1},   // 1F9D1 200D 2696
    {"🧑‍🏭", EmojiVersion::E12_1},  // 1F9D1 200D 1F527
    {"🧑‍💻", EmojiVersion::E12_1},  // 1F9D1 200D 1F4BB
    {"🧑‍🏫", EmojiVersion::E12_1},  // 1F9D1 200D 1F3EB
    {"🧑‍🌾", EmojiVersion::E12_1},  // 1F9D1 200D 1F33E
    {"🧑‍🦼", EmojiVersion::E12_1},  // 1F9D1 200D 1F9BC
    {"🧑‍🦽", EmojiVersion::E12_1},  // 1F9D1 200D 1F9BD

    // Emoji 13.0 Example
    {"🛻", EmojiVersion::E13_0},            // 1F6FB
    {"🛼", EmojiVersion::E13_0},            // 1F6FC
    {"🤵‍♀", EmojiVersion::E13_0},   // 1F935 200D 2640
    {"🤵‍♂", EmojiVersion::E13_0},   // 1F935 200D 2642
    {"🥲", EmojiVersion::E13_0},            // 1F972
    {"🥷", EmojiVersion::E13_0},            // 1F977
    {"🥸", EmojiVersion::E13_0},            // 1F978
    {"🧑‍🎄", EmojiVersion::E13_0},  // 1F9D1 200D 1F384

    // Emoji 14.0 Example
    {"🩻", EmojiVersion::E14_0},  // 1FA7B
    {"🩼", EmojiVersion::E14_0},  // 1FA7C
    {"🪩", EmojiVersion::E14_0},  // 1FAA9
    {"🪪", EmojiVersion::E14_0},  // 1FAAA
    {"🪫", EmojiVersion::E14_0},  // 1FAAB
    {"🪬", EmojiVersion::E14_0},  // 1FAAC
    {"🫃", EmojiVersion::E14_0},  // 1FAC3
    {"🫠", EmojiVersion::E14_0},  // 1FAE0
};

// This data manager overrides GetEmojiRewriterData() to return the above test
// data for EmojiRewriter.
class TestDataManager : public testing::MockDataManager {
 public:
  TestDataManager() {
    // Collect all the strings and temporarily assign 0 as index.
    absl::btree_map<std::string, size_t> string_index;
    for (const EmojiData &data : kTestEmojiList) {
      string_index[data.emoji] = 0;
    }

    // Set index.
    std::vector<absl::string_view> strings;
    size_t index = 0;
    for (auto &iter : string_index) {
      strings.push_back(iter.first);
      iter.second = index++;
    }

    // Create token array.
    for (const EmojiData &data : kTestEmojiList) {
      token_array_.push_back(0);
      token_array_.push_back(string_index[data.emoji]);
      token_array_.push_back(data.unicode_version);
      token_array_.push_back(0);
      token_array_.push_back(0);
      token_array_.push_back(0);
      token_array_.push_back(0);
    }

    // Create string array.
    string_array_data_ =
        SerializedStringArray::SerializeToBuffer(strings, &string_array_buf_);
  }

  void GetEmojiRewriterData(
      absl::string_view *token_array_data,
      absl::string_view *string_array_data) const override {
    *token_array_data =
        absl::string_view(reinterpret_cast<const char *>(token_array_.data()),
                          token_array_.size() * sizeof(uint32_t));
    *string_array_data = string_array_data_;
  }

 private:
  std::vector<uint32_t> token_array_;
  absl::string_view string_array_data_;
  std::unique_ptr<uint32_t[]> string_array_buf_;
};

class EnvironmentalFilterRewriterTest : public ::testing::Test {
 protected:
  EnvironmentalFilterRewriterTest() = default;
  ~EnvironmentalFilterRewriterTest() override = default;

  void SetUp() override {
    SystemUtil::SetUserProfileDirectory(absl::GetFlag(FLAGS_test_tmpdir));
    rewriter_ =
        std::make_unique<EnvironmentalFilterRewriter>(test_data_manager_);
  }
  std::unique_ptr<EnvironmentalFilterRewriter> rewriter_;

 private:
  const TestDataManager test_data_manager_;
};

TEST_F(EnvironmentalFilterRewriterTest, CharacterGroupFinderTest) {
  // Test for CharacterGroupFinder, with meaningless filtering target rather
  // than Emoji data. As Emoji sometimes contains un-displayed characters, this
  // test can be more explicit than using actual filtering target.
  {
    CharacterGroupFinder finder;
    finder.Initialize({
        {0x1B001},
        {0x1B002},
        {0x1B122},
        {0x1B223},
        {0x1B224},
        {0x1B225},
        {0x1B229},
        {0x1F000},
        {0x1F001},
        {0x1B111, 0x200D, 0x1B183},
        {0x1B111, 0x200D, 0x1B142, 0x200D, 0x1B924},
        {0x1B111, 0x3009},
        {0x1B142, 0x200D, 0x3009, 0x1B924},
        {0x1B924, 0x200D, 0x1B183},
    });
    EXPECT_TRUE(finder.FindMatch({0x1B001}));
    EXPECT_TRUE(finder.FindMatch({0x1B002}));
    EXPECT_TRUE(finder.FindMatch({0x1B223}));
    EXPECT_TRUE(finder.FindMatch({0x1B111, 0x200D, 0x1B142, 0x200D, 0x1B924}));
    EXPECT_TRUE(finder.FindMatch({0x1B111, 0x3009}));
    EXPECT_FALSE(finder.FindMatch({0x1B111, 0x200D, 0x1B182}));
  }
  // Test CharacterGroupFinder with Emoji data. This is also necessary to
  // express how this finder should work.
  {
    CharacterGroupFinder finder;
    finder.Initialize({
        {U'❤'},
        {U'😊'},
        {U'😋'},
        Util::Utf8ToCodepoints("🇺🇸"),
        Util::Utf8ToCodepoints("🫱🏻"),
        Util::Utf8ToCodepoints("❤️‍🔥"),
        Util::Utf8ToCodepoints("👬🏿"),
    });
    EXPECT_TRUE(finder.FindMatch(Util::Utf8ToCodepoints("これは❤です")));
    EXPECT_TRUE(finder.FindMatch(Util::Utf8ToCodepoints("これは🫱🏻です")));
    EXPECT_TRUE(finder.FindMatch(Util::Utf8ToCodepoints("これは😊です")));
    EXPECT_TRUE(finder.FindMatch(Util::Utf8ToCodepoints("これは😋です")));
    EXPECT_FALSE(
        finder.FindMatch(Util::Utf8ToCodepoints("これは😌（U+1F60C）です")));
    EXPECT_TRUE(finder.FindMatch(Util::Utf8ToCodepoints("😋これは最初です")));
    EXPECT_TRUE(finder.FindMatch(Util::Utf8ToCodepoints("これは最後です😋")));
    EXPECT_FALSE(finder.FindMatch(Util::Utf8ToCodepoints("これは🫱です")));
    EXPECT_TRUE(finder.FindMatch(Util::Utf8ToCodepoints("これは👬🏿です")));
    EXPECT_TRUE(finder.FindMatch(Util::Utf8ToCodepoints("👬🏿最初です")));
    EXPECT_TRUE(finder.FindMatch(Util::Utf8ToCodepoints("❤️‍🔥")));
    EXPECT_TRUE(finder.FindMatch(Util::Utf8ToCodepoints("最後です👬🏿")));
    EXPECT_TRUE(finder.FindMatch(Util::Utf8ToCodepoints("👬👬🏿")));
    EXPECT_FALSE(finder.FindMatch(Util::Utf8ToCodepoints("これは👬です")));
    // This is expecting to find 🇺🇸 (US). Because flag Emojis use regional
    // indicators, and they lack ZWJ between, ambiguity is inevitable. The input
    // is AUSE in regional indicators, and therefore US is found between the two
    // flags.
    EXPECT_TRUE(finder.FindMatch(Util::Utf8ToCodepoints("🇦🇺🇸🇪")));
  }
  {
    // Test with more than 16 chars.
    CharacterGroupFinder finder;
    finder.Initialize({Util::Utf8ToCodepoints("01234567890abcdefghij")});
    EXPECT_FALSE(
        finder.FindMatch(Util::Utf8ToCodepoints("01234567890abcdefghXYZ")));
  }
}

// This test aims to check the ability using EnvironmentalFilterRewriter to
// filter Emoji.
TEST_F(EnvironmentalFilterRewriterTest, EmojiFilterTest) {
  // Emoji after Unicode 12.1 should be all filtered if no additional renderable
  // charcter group is specified.
  {
    Segments segments;
    const ConversionRequest request;

    segments.Clear();
    AddSegment("a", {"🛻", "🤵‍♀", "🥸", "🧑‍🌾", "🧑‍🏭"},
               &segments);

    EXPECT_TRUE(rewriter_->Rewrite(request, &segments));
    EXPECT_EQ(0, segments.conversion_segment(0).candidates_size());
  }

  // Emoji in Unicode 13.0 should be allowed in this case.
  {
    commands::Request request;
    request.add_additional_renderable_character_groups(
        commands::Request::EMOJI_13_0);
    ConversionRequest conversion_request;
    Segments segments;
    conversion_request.set_request(&request);

    segments.Clear();
    AddSegment("a", {"🛻", "🤵‍♀", "🥸"}, &segments);

    EXPECT_FALSE(rewriter_->Rewrite(conversion_request, &segments));
    EXPECT_EQ(3, segments.conversion_segment(0).candidates_size());
  }
}

TEST_F(EnvironmentalFilterRewriterTest, RemoveTest) {
  Segments segments;
  const ConversionRequest request;

  segments.Clear();
  AddSegment("a", {"a\t1", "a\n2", "a\n\r3"}, &segments);

  EXPECT_TRUE(rewriter_->Rewrite(request, &segments));
  EXPECT_EQ(0, segments.conversion_segment(0).candidates_size());
}

TEST_F(EnvironmentalFilterRewriterTest, NoRemoveTest) {
  Segments segments;
  AddSegment("a", {"aa1", "a.a", "a-a"}, &segments);

  const ConversionRequest request;
  EXPECT_FALSE(rewriter_->Rewrite(request, &segments));
  EXPECT_EQ(3, segments.conversion_segment(0).candidates_size());
}

TEST_F(EnvironmentalFilterRewriterTest, CandidateFilterTest) {
  {
    commands::Request request;
    ConversionRequest conversion_request;
    conversion_request.set_request(&request);

    Segments segments;
    segments.Clear();
    // All should not be allowed.
    AddSegment("a",
               {kKanaSupplement_6_0, kKanaSupplement_10_0, kKanaExtendedA_14_0},
               &segments);

    EXPECT_TRUE(rewriter_->Rewrite(conversion_request, &segments));
    EXPECT_EQ(0, segments.conversion_segment(0).candidates_size());
  }

  {
    commands::Request request;
    request.add_additional_renderable_character_groups(
        commands::Request::EMPTY);
    ConversionRequest conversion_request;
    conversion_request.set_request(&request);

    Segments segments;
    segments.Clear();
    // All should not be allowed.
    AddSegment("a",
               {kKanaSupplement_6_0, kKanaSupplement_10_0, kKanaExtendedA_14_0},
               &segments);

    EXPECT_TRUE(rewriter_->Rewrite(conversion_request, &segments));
    EXPECT_EQ(0, segments.conversion_segment(0).candidates_size());
  }

  {
    commands::Request request;
    request.add_additional_renderable_character_groups(
        commands::Request::KANA_SUPPLEMENT_6_0);
    ConversionRequest conversion_request;
    conversion_request.set_request(&request);

    Segments segments;
    segments.Clear();
    // Only first one should be allowed.
    AddSegment("a",
               {kKanaSupplement_6_0, kKanaSupplement_10_0, kKanaExtendedA_14_0},
               &segments);

    EXPECT_TRUE(rewriter_->Rewrite(conversion_request, &segments));
    EXPECT_EQ(1, segments.conversion_segment(0).candidates_size());
  }

  {
    commands::Request request;
    request.add_additional_renderable_character_groups(
        commands::Request::KANA_SUPPLEMENT_6_0);
    request.add_additional_renderable_character_groups(
        commands::Request::KANA_SUPPLEMENT_AND_KANA_EXTENDED_A_10_0);
    ConversionRequest conversion_request;
    conversion_request.set_request(&request);

    Segments segments;
    segments.Clear();
    // First and second one should be allowed.
    AddSegment("a",
               {kKanaSupplement_6_0, kKanaSupplement_10_0, kKanaExtendedA_14_0},
               &segments);

    EXPECT_TRUE(rewriter_->Rewrite(conversion_request, &segments));
    EXPECT_EQ(2, segments.conversion_segment(0).candidates_size());
  }

  {
    commands::Request request;
    request.add_additional_renderable_character_groups(
        commands::Request::KANA_SUPPLEMENT_6_0);
    request.add_additional_renderable_character_groups(
        commands::Request::KANA_SUPPLEMENT_AND_KANA_EXTENDED_A_10_0);
    request.add_additional_renderable_character_groups(
        commands::Request::KANA_EXTENDED_A_14_0);
    ConversionRequest conversion_request;
    conversion_request.set_request(&request);

    Segments segments;
    segments.Clear();
    // All should be allowed.
    AddSegment("a",
               {kKanaSupplement_6_0, kKanaSupplement_10_0, kKanaExtendedA_14_0},
               &segments);

    EXPECT_FALSE(rewriter_->Rewrite(conversion_request, &segments));
    EXPECT_EQ(3, segments.conversion_segment(0).candidates_size());
  }
}

TEST_F(EnvironmentalFilterRewriterTest, FlagTest) {
  commands::Request request;
  request.mutable_decoder_experiment_params()
      ->set_enable_environmental_filter_rewriter(false);
  ConversionRequest conversion_request;
  conversion_request.set_request(&request);

  Segments segments;
  segments.Clear();
  AddSegment("test", "test", &segments);
  EXPECT_FALSE(rewriter_->Rewrite(conversion_request, &segments));
}

TEST_F(EnvironmentalFilterRewriterTest, NormalizationTest) {
  Segments segments;
  const ConversionRequest request;

  segments.Clear();
  AddSegment("test", "test", &segments);
  EXPECT_FALSE(rewriter_->Rewrite(request, &segments));
  EXPECT_EQ("test", segments.segment(0).candidate(0).value);

  segments.Clear();
  AddSegment("きょうと", "京都", &segments);
  EXPECT_FALSE(rewriter_->Rewrite(request, &segments));
  EXPECT_EQ("京都", segments.segment(0).candidate(0).value);

  // Wave dash (U+301C) per platform
  segments.Clear();
  AddSegment("なみ", "〜", &segments);
  constexpr char description[] = "[全]波ダッシュ";
  segments.mutable_segment(0)->mutable_candidate(0)->description = description;
#ifdef OS_WIN
  EXPECT_TRUE(rewriter_->Rewrite(request, &segments));
  // U+FF5E
  EXPECT_EQ("～", segments.segment(0).candidate(0).value);
  EXPECT_TRUE(segments.segment(0).candidate(0).description.empty());
#else  // OS_WIN
  EXPECT_FALSE(rewriter_->Rewrite(request, &segments));
  // U+301C
  EXPECT_EQ("〜", segments.segment(0).candidate(0).value);
  EXPECT_EQ(description, segments.segment(0).candidate(0).description);
#endif  // OS_WIN

  // Wave dash (U+301C) w/ normalization
  segments.Clear();
  AddSegment("なみ", "〜", &segments);
  segments.mutable_segment(0)->mutable_candidate(0)->description = description;

  rewriter_->SetNormalizationFlag(TextNormalizer::kAll);
  EXPECT_TRUE(rewriter_->Rewrite(request, &segments));
  // U+FF5E
  EXPECT_EQ("～", segments.segment(0).candidate(0).value);
  EXPECT_TRUE(segments.segment(0).candidate(0).description.empty());

  // Wave dash (U+301C) w/o normalization
  segments.Clear();
  AddSegment("なみ", "〜", &segments);
  segments.mutable_segment(0)->mutable_candidate(0)->description = description;

  rewriter_->SetNormalizationFlag(TextNormalizer::kNone);
  EXPECT_FALSE(rewriter_->Rewrite(request, &segments));
  // U+301C
  EXPECT_EQ("〜", segments.segment(0).candidate(0).value);
  EXPECT_EQ(description, segments.segment(0).candidate(0).description);

  // not normalized.
  segments.Clear();
  // U+301C
  AddSegment("なみ", "〜", &segments);
  segments.mutable_segment(0)->mutable_candidate(0)->attributes |=
      Segment::Candidate::USER_DICTIONARY;
  EXPECT_FALSE(rewriter_->Rewrite(request, &segments));
  // U+301C
  EXPECT_EQ("〜", segments.segment(0).candidate(0).value);

  // not normalized.
  segments.Clear();
  // U+301C
  AddSegment("なみ", "〜", &segments);
  segments.mutable_segment(0)->mutable_candidate(0)->attributes |=
      Segment::Candidate::NO_MODIFICATION;
  EXPECT_FALSE(rewriter_->Rewrite(request, &segments));
  // U+301C
  EXPECT_EQ("〜", segments.segment(0).candidate(0).value);
}

}  // namespace
}  // namespace mozc
