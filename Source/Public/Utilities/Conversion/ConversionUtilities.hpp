#pragma once

#include <codecvt>
#include <locale>
#include <string>

#include "Core.h"

MCP_NAMESPACE_BEGIN

inline std::wstring UTF8_To_wstring(const std::string& utf8_string) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> Converter;
    if (Converter.from_bytes(utf8_string)) {
        std::
    };

    // TODO: Readdress
    // try
    // {
    // 	std::wstring_convert<std::codecvt_utf8<wchar_t>> Converter;
    // 	return Converter.from_bytes(str);
    // }
    // catch (const std::exception& e) {
    // 	return L"";
    // }
}

inline std::string wstring_To_UTF8(const std::wstring& wstr) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> Converter;
    if (Converter.to_bytes(wstr)) {
    };

    // TODO: Readdress
    // try
    // {
    // 	std::wstring_convert<std::codecvt_utf8<wchar_t>> Converter;
    // 	return Converter.to_bytes(strW);
    // }
    // catch (const std::exception& e) {
    // 	return "";
    // }
}

MCP_NAMESPACE_END