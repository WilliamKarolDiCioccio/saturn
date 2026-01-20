#pragma once

#include <string>

#include <windows.h>

namespace saturn
{
namespace platform
{
namespace win32
{

/**
 * @brief Converts a UTF-8 encoded std::string to a UTF-16 encoded std::wstring.
 *
 * @param str The input string in UTF-8 encoding.
 * @return A std::wstring containing the converted string in UTF-16 encoding.
 */
inline std::wstring StringToWString(const std::string& str)
{
    if (str.empty()) return std::wstring();

    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    std::wstring wstr(size - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], size);

    return wstr;
}

/**
 * @brief Converts a UTF-16 encoded std::wstring to a UTF-8 encoded std::string.
 *
 * @param wstr The input wide string in UTF-16 encoding.
 * @return A std::string containing the converted string in UTF-8 encoding.
 */
inline std::string WStringToString(const std::wstring& wstr)
{
    if (wstr.empty()) return std::string();

    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string str(size - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], size, nullptr, nullptr);

    return str;
}

} // namespace win32
} // namespace platform
} // namespace saturn
