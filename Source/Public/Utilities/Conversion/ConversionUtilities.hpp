#pragma once


MCP_NAMESPACE_BEGIN

inline std::wstring UTF8_To_wstring(const std::string& utf8_string) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> Converter;
    if (Converter.from_bytes(utf8_string)){

    };
}

inline std::string wstring_To_UTF8(const std::wstring& wstr) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> Converter;
    if (Converter.to_bytes(wstr)){
        
    };
}

MCP_NAMESPACE_END

#pragma once

#include <string>
#include <codecvt>
#include <locale>

namespace MCP
{
	namespace StringHelper
	{
		inline std::wstring utf8_string_to_wstring(const std::string& str) 
		{
			try
			{
				std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
				return converter.from_bytes(str);
			}
			catch (const std::exception& e) {
				return L"";
			}
		}

		inline std::string wstring_to_utf8_string(const std::wstring& strW)
		{
			try 
			{
				std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
				return converter.to_bytes(strW);
			}
			catch (const std::exception& e) {
				return "";
			}
		}
	}
}