#pragma once

#include <string>
#include <variant>

#include "CoreSDK/Common/Macros.h"
#include "JSONProxy.h"

MCP_NAMESPACE_BEGIN

// ProgressToken {
//   MSG_DESCRIPTION: "A progress token, used to associate progress "
//                   "notifications with the original request.",
//                   MSG_TYPE: [ MSG_STRING, MSG_INTEGER ]
// };

// A progress token, used to associate progress notifications with the original request.
struct ProgressToken
{
	std::variant<std::string, int64_t> Token;

	ProgressToken() = default;
	explicit ProgressToken(const std::string& InValue) : Token(InValue) {}
	explicit ProgressToken(int64_t InValue) : Token(InValue) {}
	explicit ProgressToken(const std::variant<std::string, int64_t>& InValue) : Token(InValue) {}

	[[nodiscard]] std::string ToString() const
	{
		return std::visit(
			[]<typename T0>(const T0& InValue) -> std::string
			{
				if constexpr (std::is_same_v<std::decay_t<T0>, std::string>)
				{
					return InValue;
				}
				else
				{
					return std::to_string(InValue);
				}
			},
			Token);
	}

	template <typename BasicJSONData> friend void to_json(BasicJSONData& InJSON, const ProgressToken& InProgressToken)
	{
		InJSON = InProgressToken.ToString();
	}

	template <typename BasicJSONData> friend void from_json(const BasicJSONData& InJSON, ProgressToken& InProgressToken)
	{
		if (InJSON.is_string())
		{
			InProgressToken.Token = InJSON.template get<std::string>();
		}
		else if (InJSON.is_number_integer())
		{
			InProgressToken.Token = InJSON.template get<int64_t>();
		}
		else
		{
			throw std::invalid_argument("ProgressToken must be string or integer");
		}
	}
};

MCP_NAMESPACE_END
