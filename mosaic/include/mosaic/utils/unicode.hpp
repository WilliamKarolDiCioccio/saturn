#pragma once

#include <string>
#include <vector>
#include <optional>

#include <unicode/unistr.h>
#include <unicode/utypes.h>
#include <unicode/uchar.h>

#include <pieces/result.hpp>

namespace mosaic
{
namespace utils
{

/**
 * @brief Converts a UTF-8 encoded string to a vector of Unicode codepoints (char32_t).
 *
 * @param utf8 The UTF-8 encoded string to convert.
 * @param outCodepoints The output vector that will contain the Unicode codepoints.
 * @return true if the conversion was successful, meaning the input was valid UTF-8 and the
 * codepoints were extracted.
 * @return false if there was an error during conversion, such as invalid UTF-8 input.
 */
inline std::vector<char32_t> Utf8ToCodepoints(const std::string& _utf8)
{
    std::vector<char32_t> outCodepoints;

    icu::UnicodeString ustr = icu::UnicodeString::fromUTF8(_utf8);

    int32_t len = ustr.length();

    for (int32_t i = 0; i < len;)
    {
        UChar32 cp = ustr.char32At(i);
        outCodepoints.push_back(static_cast<char32_t>(cp));
        i += U16_LENGTH(cp); // advance by number of UTF-16 code units
    }

    return outCodepoints;
}

/**
 * @brief Converts a vector of Unicode codepoints (char32_t) to a UTF-8 encoded string.
 *
 * @param codepoints The vector of Unicode codepoints to convert.
 * @return std::string The resulting UTF-8 encoded string.
 */
inline std::string CodepointsToUtf8(const std::vector<char32_t>& _codepoints)
{
    icu::UnicodeString ustr;

    for (char32_t cp : _codepoints)
    {
        ustr.append(static_cast<UChar32>(cp));
    }

    std::string result;
    ustr.toUTF8String(result);
    return result;
}

/**
 * @brief Converts a UTF-8 encoded string to an ICU UnicodeString.
 *
 * @param utf8 The UTF-8 encoded string to convert.
 * @param outStr The output ICU UnicodeString that will contain the converted string.
 * @return true if the conversion was successful, meaning the input was valid UTF-8 and the
 * @return false if there was an error during conversion, such as invalid UTF-8 input.
 */
inline icu::UnicodeString Utf8ToUnicodeString(const std::string& _utf8)
{
    return icu::UnicodeString::fromUTF8(_utf8);
}

/**
 * @brief Converts an ICU UnicodeString to a UTF-8 encoded string.
 *
 * @param ustr The ICU UnicodeString to convert.
 * @return std::string The resulting UTF-8 encoded string.
 */
inline std::string UnicodeStringToUtf8(const icu::UnicodeString& _ustr)
{
    std::string result;
    _ustr.toUTF8String(result);
    return result;
}

/**
 * @brief Converts an ICU UnicodeString to a vector of Unicode codepoints (char32_t).
 *
 * @param ustr The ICU UnicodeString to convert.
 * @param outCodepoints The output vector that will contain the Unicode codepoints.
 */
inline std::vector<char32_t> UnicodeStringToCodepoints(const icu::UnicodeString& _ustr)
{
    std::vector<char32_t> outCodepoints;

    int32_t len = _ustr.length();

    for (int32_t i = 0; i < len;)
    {
        UChar32 cp = _ustr.char32At(i);
        outCodepoints.push_back(static_cast<char32_t>(cp));
        i += U16_LENGTH(cp);
    }

    return outCodepoints;
}

/**
 * @brief Converts a vector of Unicode codepoints (char32_t) to an ICU UnicodeString.
 *
 * @param codepoints The vector of Unicode codepoints to convert.
 * @return icu::UnicodeString The resulting ICU UnicodeString.
 */
inline icu::UnicodeString CodepointsToUnicodeString(const std::vector<char32_t>& _codepoints)
{
    icu::UnicodeString ustr;

    for (char32_t cp : _codepoints)
    {
        ustr.append(static_cast<UChar32>(cp));
    }

    return ustr;
}

} // namespace utils
} // namespace mosaic
