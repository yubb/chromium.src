// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>

#include "base/auto_reset.h"
#include "base/bind.h"
#include "base/memory/scoped_ptr.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/autocomplete/scored_history_match_builder_impl.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::ASCIIToUTF16;

namespace {

// Returns a history::VisitInfoVector that includes |num_visits| spread over the
// last |frequency|*|num_visits| days (relative to |now|).  A frequency of
// one means one visit each day, two means every other day, etc.
history::VisitInfoVector CreateVisitInfoVector(int num_visits,
                                               int frequency,
                                               base::Time now) {
  history::VisitInfoVector visits;
  for (int i = 0; i < num_visits; ++i) {
    visits.push_back(
        std::make_pair(now - base::TimeDelta::FromDays(i * frequency),
                       ui::PAGE_TRANSITION_LINK));
  }
  return visits;
}

}  // namespace

class ScoredHistoryMatchBuilderImplTest : public testing::Test {
 protected:
  // Convenience function to create a history::URLRow with basic data for |url|,
  // |title|, |visit_count|, and |typed_count|. |days_since_last_visit| gives
  // the number of days ago to which to set the URL's last_visit.
  history::URLRow MakeURLRow(const char* url,
                             const char* title,
                             int visit_count,
                             int days_since_last_visit,
                             int typed_count);

  // Convenience function to set the word starts information from a
  // history::URLRow's URL and title.
  void PopulateWordStarts(const history::URLRow& url_row,
                          history::RowWordStarts* word_starts);

  // Convenience functions for easily creating vectors of search terms.
  history::String16Vector Make1Term(const char* term) const;
  history::String16Vector Make2Terms(const char* term_1,
                                     const char* term_2) const;

  // Convenience function for GetTopicalityScore() that builds the term match
  // and word break information automatically that are needed to call
  // GetTopicalityScore().  It only works for scoring a single term, not
  // multiple terms.
  float GetTopicalityScoreOfTermAgainstURLAndTitle(const base::string16& term,
                                                   const base::string16& url,
                                                   const base::string16& title);

  // Registers an URL as bookmarked.
  void AddBookmark(const GURL& url);

  // Returns the history::ScoredHistoryMatch::Builder to use.
  history::ScoredHistoryMatch::Builder* builder() { return builder_.get(); }

 private:
  // Returns whether an URL is bookmarked, used by
  // ScoredHistoryMatchBuilderImpl.
  bool IsBookmarked(const GURL& url);

  // testing::Test implementation.
  void SetUp() override;
  void TearDown() override;

  std::set<GURL> bookmarked_urls_;
  scoped_ptr<history::ScoredHistoryMatch::Builder> builder_;
};

history::URLRow ScoredHistoryMatchBuilderImplTest::MakeURLRow(
    const char* url,
    const char* title,
    int visit_count,
    int days_since_last_visit,
    int typed_count) {
  history::URLRow row(GURL(url), 0);
  row.set_title(ASCIIToUTF16(title));
  row.set_visit_count(visit_count);
  row.set_typed_count(typed_count);
  row.set_last_visit(base::Time::NowFromSystemTime() -
                     base::TimeDelta::FromDays(days_since_last_visit));
  return row;
}

void ScoredHistoryMatchBuilderImplTest::PopulateWordStarts(
    const history::URLRow& url_row,
    history::RowWordStarts* word_starts) {
  history::String16SetFromString16(ASCIIToUTF16(url_row.url().spec()),
                                   &word_starts->url_word_starts_);
  history::String16SetFromString16(url_row.title(),
                                   &word_starts->title_word_starts_);
}

history::String16Vector ScoredHistoryMatchBuilderImplTest::Make1Term(
    const char* term) const {
  history::String16Vector original_terms;
  original_terms.push_back(ASCIIToUTF16(term));
  return original_terms;
}

history::String16Vector ScoredHistoryMatchBuilderImplTest::Make2Terms(
    const char* term_1,
    const char* term_2) const {
  history::String16Vector original_terms;
  original_terms.push_back(ASCIIToUTF16(term_1));
  original_terms.push_back(ASCIIToUTF16(term_2));
  return original_terms;
}

float ScoredHistoryMatchBuilderImplTest::
    GetTopicalityScoreOfTermAgainstURLAndTitle(const base::string16& term,
                                               const base::string16& url,
                                               const base::string16& title) {
  // Make an empty match and simply populate the fields we need in order
  // to call GetTopicalityScore().
  history::ScoredHistoryMatch scored_match;
  scored_match.url_matches = history::MatchTermInString(term, url, 0);
  scored_match.title_matches = history::MatchTermInString(term, title, 0);
  history::RowWordStarts word_starts;
  history::String16SetFromString16(url, &word_starts.url_word_starts_);
  history::String16SetFromString16(title, &word_starts.title_word_starts_);
  history::WordStarts one_word_no_offset(1, 0u);
  return ScoredHistoryMatchBuilderImpl::GetTopicalityScore(
      1, url, one_word_no_offset, word_starts, &scored_match);
}

void ScoredHistoryMatchBuilderImplTest::AddBookmark(const GURL& url) {
  bookmarked_urls_.insert(url);
}

bool ScoredHistoryMatchBuilderImplTest::IsBookmarked(const GURL& url) {
  return bookmarked_urls_.find(url) != bookmarked_urls_.end();
}

void ScoredHistoryMatchBuilderImplTest::SetUp() {
  builder_.reset(new ScoredHistoryMatchBuilderImpl(
      base::Bind(&ScoredHistoryMatchBuilderImplTest::IsBookmarked,
                 base::Unretained(this))));
}

void ScoredHistoryMatchBuilderImplTest::TearDown() {
  bookmarked_urls_.clear();
  builder_.reset();
}

TEST_F(ScoredHistoryMatchBuilderImplTest, Scoring) {
  // We use NowFromSystemTime() because MakeURLRow uses the same function
  // to calculate last visit time when building a row.
  base::Time now = base::Time::NowFromSystemTime();

  history::URLRow row_a(MakeURLRow("http://fedcba", "abcd bcd", 3, 30, 1));
  history::RowWordStarts word_starts_a;
  PopulateWordStarts(row_a, &word_starts_a);
  history::WordStarts one_word_no_offset(1, 0u);
  history::VisitInfoVector visits_a = CreateVisitInfoVector(3, 30, now);
  // Mark one visit as typed.
  visits_a[0].second = ui::PAGE_TRANSITION_TYPED;
  history::ScoredHistoryMatch scored_a = builder()->Build(
      row_a, visits_a, std::string(), ASCIIToUTF16("abc"), Make1Term("abc"),
      one_word_no_offset, word_starts_a, now);

  // Test scores based on visit_count.
  history::URLRow row_b(MakeURLRow("http://abcdef", "abcd bcd", 10, 30, 1));
  history::RowWordStarts word_starts_b;
  PopulateWordStarts(row_b, &word_starts_b);
  history::VisitInfoVector visits_b = CreateVisitInfoVector(10, 30, now);
  visits_b[0].second = ui::PAGE_TRANSITION_TYPED;
  history::ScoredHistoryMatch scored_b = builder()->Build(
      row_b, visits_b, std::string(), ASCIIToUTF16("abc"), Make1Term("abc"),
      one_word_no_offset, word_starts_b, now);
  EXPECT_GT(scored_b.raw_score, scored_a.raw_score);

  // Test scores based on last_visit.
  history::URLRow row_c(MakeURLRow("http://abcdef", "abcd bcd", 3, 10, 1));
  history::RowWordStarts word_starts_c;
  PopulateWordStarts(row_c, &word_starts_c);
  history::VisitInfoVector visits_c = CreateVisitInfoVector(3, 10, now);
  visits_c[0].second = ui::PAGE_TRANSITION_TYPED;
  history::ScoredHistoryMatch scored_c = builder()->Build(
      row_c, visits_c, std::string(), ASCIIToUTF16("abc"), Make1Term("abc"),
      one_word_no_offset, word_starts_c, now);
  EXPECT_GT(scored_c.raw_score, scored_a.raw_score);

  // Test scores based on typed_count.
  history::URLRow row_d(MakeURLRow("http://abcdef", "abcd bcd", 3, 30, 3));
  history::RowWordStarts word_starts_d;
  PopulateWordStarts(row_d, &word_starts_d);
  history::VisitInfoVector visits_d = CreateVisitInfoVector(3, 30, now);
  visits_d[0].second = ui::PAGE_TRANSITION_TYPED;
  visits_d[1].second = ui::PAGE_TRANSITION_TYPED;
  visits_d[2].second = ui::PAGE_TRANSITION_TYPED;
  history::ScoredHistoryMatch scored_d = builder()->Build(
      row_d, visits_d, std::string(), ASCIIToUTF16("abc"), Make1Term("abc"),
      one_word_no_offset, word_starts_d, now);
  EXPECT_GT(scored_d.raw_score, scored_a.raw_score);

  // Test scores based on a terms appearing multiple times.
  history::URLRow row_e(MakeURLRow(
      "http://csi.csi.csi/csi_csi",
      "CSI Guide to CSI Las Vegas, CSI New York, CSI Provo", 3, 30, 3));
  history::RowWordStarts word_starts_e;
  PopulateWordStarts(row_e, &word_starts_e);
  const history::VisitInfoVector visits_e = visits_d;
  history::ScoredHistoryMatch scored_e = builder()->Build(
      row_e, visits_e, std::string(), ASCIIToUTF16("csi"), Make1Term("csi"),
      one_word_no_offset, word_starts_e, now);
  EXPECT_LT(scored_e.raw_score, 1400);

  // Test that a result with only a mid-term match (i.e., not at a word
  // boundary) scores 0.
  history::ScoredHistoryMatch scored_f =
      builder()->Build(row_a, visits_a, std::string(), ASCIIToUTF16("cd"),
                       Make1Term("cd"), one_word_no_offset, word_starts_a, now);
  EXPECT_EQ(scored_f.raw_score, 0);
}

TEST_F(ScoredHistoryMatchBuilderImplTest, ScoringBookmarks) {
  // We use NowFromSystemTime() because MakeURLRow uses the same function
  // to calculate last visit time when building a row.
  base::Time now = base::Time::NowFromSystemTime();

  std::string url_string("http://fedcba");
  const GURL url(url_string);
  history::URLRow row(MakeURLRow(url_string.c_str(), "abcd bcd", 8, 3, 1));
  history::RowWordStarts word_starts;
  PopulateWordStarts(row, &word_starts);
  history::WordStarts one_word_no_offset(1, 0u);
  history::VisitInfoVector visits = CreateVisitInfoVector(8, 3, now);
  history::ScoredHistoryMatch scored =
      builder()->Build(row, visits, std::string(), ASCIIToUTF16("abc"),
                       Make1Term("abc"), one_word_no_offset, word_starts, now);
  // Now bookmark that URL and make sure its score increases.
  base::AutoReset<int> reset(&ScoredHistoryMatchBuilderImpl::bookmark_value_,
                             5);
  AddBookmark(url);
  history::ScoredHistoryMatch scored_with_bookmark =
      builder()->Build(row, visits, std::string(), ASCIIToUTF16("abc"),
                       Make1Term("abc"), one_word_no_offset, word_starts, now);
  EXPECT_GT(scored_with_bookmark.raw_score, scored.raw_score);
}

TEST_F(ScoredHistoryMatchBuilderImplTest, ScoringTLD) {
  // We use NowFromSystemTime() because MakeURLRow uses the same function
  // to calculate last visit time when building a row.
  base::Time now = base::Time::NowFromSystemTime();

  // By default the URL should not be returned for a query that includes "com".
  std::string url_string("http://fedcba.com/");
  const GURL url(url_string);
  history::URLRow row(MakeURLRow(url_string.c_str(), "", 8, 3, 1));
  history::RowWordStarts word_starts;
  PopulateWordStarts(row, &word_starts);
  history::WordStarts two_words_no_offsets(2, 0u);
  history::VisitInfoVector visits = CreateVisitInfoVector(8, 3, now);
  history::ScoredHistoryMatch scored = builder()->Build(
      row, visits, std::string(), ASCIIToUTF16("fed com"),
      Make2Terms("fed", "com"), two_words_no_offsets, word_starts, now);
  EXPECT_EQ(0, scored.raw_score);

  // Now allow credit for the match in the TLD.
  base::AutoReset<bool> reset(
      &ScoredHistoryMatchBuilderImpl::allow_tld_matches_, true);
  history::ScoredHistoryMatch scored_with_tld = builder()->Build(
      row, visits, std::string(), ASCIIToUTF16("fed com"),
      Make2Terms("fed", "com"), two_words_no_offsets, word_starts, now);
  EXPECT_GT(scored_with_tld.raw_score, 0);
}

TEST_F(ScoredHistoryMatchBuilderImplTest, ScoringScheme) {
  // We use NowFromSystemTime() because MakeURLRow uses the same function
  // to calculate last visit time when building a row.
  base::Time now = base::Time::NowFromSystemTime();

  // By default the URL should not be returned for a query that includes "http".
  std::string url_string("http://fedcba/");
  const GURL url(url_string);
  history::URLRow row(MakeURLRow(url_string.c_str(), "", 8, 3, 1));
  history::RowWordStarts word_starts;
  PopulateWordStarts(row, &word_starts);
  history::WordStarts two_words_no_offsets(2, 0u);
  history::VisitInfoVector visits = CreateVisitInfoVector(8, 3, now);
  history::ScoredHistoryMatch scored = builder()->Build(
      row, visits, std::string(), ASCIIToUTF16("fed http"),
      Make2Terms("fed", "http"), two_words_no_offsets, word_starts, now);
  EXPECT_EQ(0, scored.raw_score);

  // Now allow credit for the match in the scheme.
  base::AutoReset<bool> reset(
      &ScoredHistoryMatchBuilderImpl::allow_scheme_matches_, true);
  history::ScoredHistoryMatch scored_with_scheme = builder()->Build(
      row, visits, std::string(), ASCIIToUTF16("fed http"),
      Make2Terms("fed", "http"), two_words_no_offsets, word_starts, now);
  EXPECT_GT(scored_with_scheme.raw_score, 0);
}

TEST_F(ScoredHistoryMatchBuilderImplTest, Inlining) {
  // We use NowFromSystemTime() because MakeURLRow uses the same function
  // to calculate last visit time when building a row.
  base::Time now = base::Time::NowFromSystemTime();
  history::RowWordStarts word_starts;
  history::WordStarts one_word_no_offset(1, 0u);
  history::VisitInfoVector visits;

  {
    history::URLRow row(
        MakeURLRow("http://www.google.com", "abcdef", 3, 30, 1));
    PopulateWordStarts(row, &word_starts);
    history::ScoredHistoryMatch scored_a =
        builder()->Build(row, visits, std::string(), ASCIIToUTF16("g"),
                         Make1Term("g"), one_word_no_offset, word_starts, now);
    EXPECT_TRUE(scored_a.can_inline);
    EXPECT_FALSE(scored_a.match_in_scheme);
    history::ScoredHistoryMatch scored_b =
        builder()->Build(row, visits, std::string(), ASCIIToUTF16("w"),
                         Make1Term("w"), one_word_no_offset, word_starts, now);
    EXPECT_TRUE(scored_b.can_inline);
    EXPECT_FALSE(scored_b.match_in_scheme);
    history::ScoredHistoryMatch scored_c =
        builder()->Build(row, visits, std::string(), ASCIIToUTF16("h"),
                         Make1Term("h"), one_word_no_offset, word_starts, now);
    EXPECT_TRUE(scored_c.can_inline);
    EXPECT_TRUE(scored_c.match_in_scheme);
    history::ScoredHistoryMatch scored_d =
        builder()->Build(row, visits, std::string(), ASCIIToUTF16("o"),
                         Make1Term("o"), one_word_no_offset, word_starts, now);
    EXPECT_FALSE(scored_d.can_inline);
    EXPECT_FALSE(scored_d.match_in_scheme);
  }

  {
    history::URLRow row(MakeURLRow("http://teams.foo.com", "abcdef", 3, 30, 1));
    PopulateWordStarts(row, &word_starts);
    history::ScoredHistoryMatch scored_a =
        builder()->Build(row, visits, std::string(), ASCIIToUTF16("t"),
                         Make1Term("t"), one_word_no_offset, word_starts, now);
    EXPECT_TRUE(scored_a.can_inline);
    EXPECT_FALSE(scored_a.match_in_scheme);
    history::ScoredHistoryMatch scored_b =
        builder()->Build(row, visits, std::string(), ASCIIToUTF16("f"),
                         Make1Term("f"), one_word_no_offset, word_starts, now);
    EXPECT_FALSE(scored_b.can_inline);
    EXPECT_FALSE(scored_b.match_in_scheme);
    history::ScoredHistoryMatch scored_c =
        builder()->Build(row, visits, std::string(), ASCIIToUTF16("o"),
                         Make1Term("o"), one_word_no_offset, word_starts, now);
    EXPECT_FALSE(scored_c.can_inline);
    EXPECT_FALSE(scored_c.match_in_scheme);
  }

  {
    history::URLRow row(
        MakeURLRow("https://www.testing.com", "abcdef", 3, 30, 1));
    PopulateWordStarts(row, &word_starts);
    history::ScoredHistoryMatch scored_a =
        builder()->Build(row, visits, std::string(), ASCIIToUTF16("t"),
                         Make1Term("t"), one_word_no_offset, word_starts, now);
    EXPECT_TRUE(scored_a.can_inline);
    EXPECT_FALSE(scored_a.match_in_scheme);
    history::ScoredHistoryMatch scored_b =
        builder()->Build(row, visits, std::string(), ASCIIToUTF16("h"),
                         Make1Term("h"), one_word_no_offset, word_starts, now);
    EXPECT_TRUE(scored_b.can_inline);
    EXPECT_TRUE(scored_b.match_in_scheme);
    history::ScoredHistoryMatch scored_c =
        builder()->Build(row, visits, std::string(), ASCIIToUTF16("w"),
                         Make1Term("w"), one_word_no_offset, word_starts, now);
    EXPECT_TRUE(scored_c.can_inline);
    EXPECT_FALSE(scored_c.match_in_scheme);
  }
}

TEST_F(ScoredHistoryMatchBuilderImplTest, GetTopicalityScoreTrailingSlash) {
  const float hostname = GetTopicalityScoreOfTermAgainstURLAndTitle(
      ASCIIToUTF16("def"), ASCIIToUTF16("http://abc.def.com/"),
      ASCIIToUTF16("Non-Matching Title"));
  const float hostname_no_slash = GetTopicalityScoreOfTermAgainstURLAndTitle(
      ASCIIToUTF16("def"), ASCIIToUTF16("http://abc.def.com"),
      ASCIIToUTF16("Non-Matching Title"));
  EXPECT_EQ(hostname_no_slash, hostname);
}

// This function only tests scoring of single terms that match exactly
// once somewhere in the URL or title.
TEST_F(ScoredHistoryMatchBuilderImplTest, GetTopicalityScore) {
  base::string16 url = ASCIIToUTF16(
      "http://abc.def.com/path1/path2?"
      "arg1=val1&arg2=val2#hash_component");
  base::string16 title = ASCIIToUTF16("here is a title");
  const float hostname_score =
      GetTopicalityScoreOfTermAgainstURLAndTitle(
          ASCIIToUTF16("abc"), url, title);
  const float hostname_mid_word_score =
      GetTopicalityScoreOfTermAgainstURLAndTitle(
          ASCIIToUTF16("bc"), url, title);
  const float domain_name_score =
      GetTopicalityScoreOfTermAgainstURLAndTitle(
          ASCIIToUTF16("def"), url, title);
  const float domain_name_mid_word_score =
      GetTopicalityScoreOfTermAgainstURLAndTitle(
          ASCIIToUTF16("ef"), url, title);
  const float tld_score =
      GetTopicalityScoreOfTermAgainstURLAndTitle(
          ASCIIToUTF16("com"), url, title);
  const float tld_mid_word_score =
      GetTopicalityScoreOfTermAgainstURLAndTitle(
          ASCIIToUTF16("om"), url, title);
  const float path_score =
      GetTopicalityScoreOfTermAgainstURLAndTitle(
          ASCIIToUTF16("path1"), url, title);
  const float path_mid_word_score =
      GetTopicalityScoreOfTermAgainstURLAndTitle(
          ASCIIToUTF16("ath1"), url, title);
  const float arg_score =
      GetTopicalityScoreOfTermAgainstURLAndTitle(
          ASCIIToUTF16("arg2"), url, title);
  const float arg_mid_word_score =
      GetTopicalityScoreOfTermAgainstURLAndTitle(
          ASCIIToUTF16("rg2"), url, title);
  const float protocol_score =
      GetTopicalityScoreOfTermAgainstURLAndTitle(
          ASCIIToUTF16("htt"), url, title);
  const float protocol_mid_word_score =
      GetTopicalityScoreOfTermAgainstURLAndTitle(
          ASCIIToUTF16("tt"), url, title);
  const float title_score =
      GetTopicalityScoreOfTermAgainstURLAndTitle(
          ASCIIToUTF16("her"), url, title);
  const float title_mid_word_score =
      GetTopicalityScoreOfTermAgainstURLAndTitle(
          ASCIIToUTF16("er"), url, title);
  // Verify hostname and domain name > path > arg.
  EXPECT_GT(hostname_score, path_score);
  EXPECT_GT(domain_name_score, path_score);
  EXPECT_GT(path_score, arg_score);
  // Verify that domain name > path and domain name > arg for non-word
  // boundaries.
  EXPECT_GT(hostname_mid_word_score, path_mid_word_score);
  EXPECT_GT(domain_name_mid_word_score, path_mid_word_score);
  EXPECT_GT(domain_name_mid_word_score, arg_mid_word_score);
  EXPECT_GT(hostname_mid_word_score, arg_mid_word_score);
  // Also verify that the matches at non-word-boundaries all score
  // worse than the matches at word boundaries.  These three sets suffice.
  EXPECT_GT(arg_score, hostname_mid_word_score);
  EXPECT_GT(arg_score, domain_name_mid_word_score);
  EXPECT_GT(title_score, title_mid_word_score);
  // Check that title matches fit somewhere reasonable compared to the
  // various types of URL matches.
  EXPECT_GT(title_score, arg_score);
  EXPECT_GT(arg_score, title_mid_word_score);
  // Finally, verify that protocol matches and top level domain name
  // matches (.com, .net, etc.) score worse than some of the mid-word
  // matches that actually count.
  EXPECT_GT(hostname_mid_word_score, protocol_score);
  EXPECT_GT(hostname_mid_word_score, protocol_mid_word_score);
  EXPECT_GT(hostname_mid_word_score, tld_score);
  EXPECT_GT(hostname_mid_word_score, tld_mid_word_score);
}
