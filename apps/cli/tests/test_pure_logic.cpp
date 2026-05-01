#include <gtest/gtest.h>

#include <optional>
#include <string>

#include "console-utils.h"
#include "date-utils.h"
#include "interactive-table.h"

namespace {

// ---- utf8_len ---------------------------------------------------------------

TEST(Utf8Len, Empty) {
    EXPECT_EQ(utf8_len(""), 0u);
}

TEST(Utf8Len, AsciiOnly) {
    EXPECT_EQ(utf8_len("abc"), 3u);
    EXPECT_EQ(utf8_len("hello world"), 11u);
}

TEST(Utf8Len, MultibyteCyrillic) {
    // "Hello" in Russian: each Cyrillic letter is 2 bytes in UTF-8 → 6 chars,
    // 12 bytes; utf8_len must report 6.
    const std::string ru = "\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82";
    ASSERT_EQ(ru.size(), 12u);
    EXPECT_EQ(utf8_len(ru), 6u);
}

TEST(Utf8Len, FourByteEmoji) {
    // 🌱 = U+1F331, 4 bytes in UTF-8.
    const std::string seedling = "\xF0\x9F\x8C\xB1";
    ASSERT_EQ(seedling.size(), 4u);
    EXPECT_EQ(utf8_len(seedling), 1u);
}

TEST(Utf8Len, MixedAsciiAndMultibyte) {
    // "ab" + "ё" (2 bytes) + "c" = 4 visible chars.
    const std::string mixed = std::string("ab") + "\xD1\x91" + "c";
    EXPECT_EQ(utf8_len(mixed), 4u);
}

// ---- calculateAge -----------------------------------------------------------
//
// `time(nullptr)` is not injectable, so we cannot assert exact age. We assert
// the contract: invalid inputs return "-", valid past dates return one of the
// expected suffixes ("years"/"months"/"days").

TEST(CalculateAge, EmptyOptional) {
    EXPECT_EQ(calculateAge(std::nullopt), "-");
}

TEST(CalculateAge, EmptyString) {
    EXPECT_EQ(calculateAge(std::optional<std::string>("")), "-");
}

TEST(CalculateAge, MalformedString) {
    EXPECT_EQ(calculateAge(std::optional<std::string>("not-a-date")), "-");
    EXPECT_EQ(calculateAge(std::optional<std::string>("2020/01/15")), "-");
    EXPECT_EQ(calculateAge(std::optional<std::string>("2020-01")), "-");
}

TEST(CalculateAge, OutOfRangeMonthOrDay) {
    EXPECT_EQ(calculateAge(std::optional<std::string>("2020-13-01")), "-");
    EXPECT_EQ(calculateAge(std::optional<std::string>("2020-00-15")), "-");
    EXPECT_EQ(calculateAge(std::optional<std::string>("2020-06-32")), "-");
    EXPECT_EQ(calculateAge(std::optional<std::string>("2020-06-00")), "-");
}

TEST(CalculateAge, OutOfRangeYear) {
    EXPECT_EQ(calculateAge(std::optional<std::string>("1899-01-01")), "-");
    EXPECT_EQ(calculateAge(std::optional<std::string>("2101-01-01")), "-");
}

TEST(CalculateAge, ValidPastDateReturnsKnownSuffix) {
    const auto out = calculateAge(std::optional<std::string>("2020-01-15"));
    ASSERT_NE(out, "-");
    const bool ok =
        out.find(" years") != std::string::npos ||
        out.find(" months") != std::string::npos ||
        out.find(" days") != std::string::npos;
    EXPECT_TRUE(ok) << "unexpected age format: " << out;
}

TEST(CalculateAge, BoundaryYears) {
    EXPECT_NE(calculateAge(std::optional<std::string>("1900-01-01")), "-");
    EXPECT_NE(calculateAge(std::optional<std::string>("2100-12-31")), "-");
}

// ---- waterDateOnly / timesWordRu / waterFreqText (from interactive-table.inl)

TEST(WaterDateOnly, EmptyReturnsDash) {
    EXPECT_EQ(waterDateOnly(""), "-");
}

TEST(WaterDateOnly, FullTimestampTrimmedToTenChars) {
    EXPECT_EQ(waterDateOnly("2026-04-20T12:34:56+00:00"), "2026-04-20");
    EXPECT_EQ(waterDateOnly("2026-04-20"), "2026-04-20");
}

TEST(WaterDateOnly, ShortStringReturnedAsIs) {
    EXPECT_EQ(waterDateOnly("short"), "short");
}

TEST(TimesWordRu, SingularAndPluralBoundaries) {
    EXPECT_EQ(timesWordRu(1), "time");
    EXPECT_EQ(timesWordRu(2), "times");
    EXPECT_EQ(timesWordRu(4), "times");
    EXPECT_EQ(timesWordRu(5), "times");
    EXPECT_EQ(timesWordRu(11), "times");
    EXPECT_EQ(timesWordRu(14), "times");
    EXPECT_EQ(timesWordRu(21), "time");
    EXPECT_EQ(timesWordRu(22), "times");
    EXPECT_EQ(timesWordRu(100), "times");
    EXPECT_EQ(timesWordRu(101), "time");
}

TEST(WaterFreqText, NotSet) {
    Water w{};
    w.frequency = -1;
    w.frequencyMeasure = "";
    EXPECT_EQ(waterFreqText(w), "-");
}

TEST(WaterFreqText, ZeroAlsoDash) {
    Water w{};
    w.frequency = 0;
    w.frequencyMeasure = "days";
    EXPECT_EQ(waterFreqText(w), "-");
}

TEST(WaterFreqText, MeasureMissing) {
    Water w{};
    w.frequency = 1;
    w.frequencyMeasure = "";
    EXPECT_EQ(waterFreqText(w), "1 time");
}

TEST(WaterFreqText, WithMeasure) {
    Water w{};
    w.frequency = 3;
    w.frequencyMeasure = "days";
    EXPECT_EQ(waterFreqText(w), "3 times days");
}

TEST(WaterFreqText, SingularWithMeasure) {
    Water w{};
    w.frequency = 21;
    w.frequencyMeasure = "weeks";
    EXPECT_EQ(waterFreqText(w), "21 time weeks");
}

}  // namespace
