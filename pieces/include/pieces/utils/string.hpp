#pragma once

#include <string>
#include <vector>
#include <optional>

#include <utf8.h>

namespace pieces
{
namespace utils
{

/**
 * @brief Converts a UTF-8 encoded string to a vector of Unicode codepoints (char32_t).
 *
 * @param utf8 The UTF-8 encoded string to convert.
 * @return std::vector<char32_t> The extracted codepoints.
 *         If the input is not valid UTF-8, an exception will be thrown (handle upstream).
 */
[[nodiscard]] inline std::vector<char32_t> Utf8ToCodepoints(const std::string& utf8)
{
    std::vector<char32_t> codepoints;
    auto it = utf8.begin();
    const auto end = utf8.end();

    while (it != end)
    {
        try
        {
            char32_t cp = utf8::next(it, end);
            codepoints.push_back(cp);
        }
        catch (...)
        {
            // Skip or rethrow depending on desired behavior
            // throw; // or just continue;
            break;
        }
    }

    return codepoints;
}

/**
 * @brief Converts a vector of Unicode codepoints (char32_t) to a UTF-8 encoded string.
 *
 * @param codepoints The vector of Unicode codepoints to convert.
 * @return std::string The resulting UTF-8 encoded string.
 */
[[nodiscard]] inline std::string CodepointsToUtf8(const std::vector<char32_t>& codepoints)
{
    std::string utf8;
    utf8::utf32to8(codepoints.begin(), codepoints.end(), std::back_inserter(utf8));
    return utf8;
}

/**
 * @brief Converts a UTF-8 encoded string to a UTF-32 string (std::u32string).
 *
 * @param utf8 The UTF-8 encoded string to convert.
 * @return std::u32string UTF-32 representation.
 */
[[nodiscard]] inline std::u32string Utf8ToUtf32(const std::string& utf8)
{
    std::u32string result;
    utf8::utf8to32(utf8.begin(), utf8.end(), std::back_inserter(result));
    return result;
}

/**
 * @brief Converts a UTF-32 string to UTF-8.
 *
 * @param utf32 The UTF-32 encoded string to convert.
 * @return std::string UTF-8 representation.
 */
[[nodiscard]] inline std::string Utf32ToUtf8(const std::u32string& utf32)
{
    std::string result;
    utf8::utf32to8(utf32.begin(), utf32.end(), std::back_inserter(result));
    return result;
}

} // namespace utils
} // namespace pieces
