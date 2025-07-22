#pragma once

#include <optional>
#include <vector>

#include "CoreSDK/Common/BaseTypes.h"
#include "CoreSDK/Common/Macros.h"
#include "CoreSDK/Common/Roles.h"
#include "JSONProxy.h"

MCP_NAMESPACE_BEGIN

// FAnnotations {
//     MSG_DESCRIPTION: "Optional annotations for the client. The client can use annotations to inform how objects are
//     used or displayed",
//     MSG_PROPERTIES: {
//             MSG_AUDIENCE: {
//                 MSG_DESCRIPTION:
//                     "Describes who the intended customer of this object or data is.
//                     It can include multiple entries to indicate content useful for multiple audiences (e.g.,
//                     `[\"user\", \"assistant\"]`).",
//                 MSG_ITEMS: {"$ref": "#/definitions/ERole"},
//                 MSG_TYPE: MSG_ARRAY
//             },
//             MSG_PRIORITY: {
//                 MSG_DESCRIPTION: "Describes how important this data is for operating the server.
//                 A value of 1 means 'most important', and indicates that the data is effectively required, while 0
//                 means 'least important', and indicates that the data is entirely optional.",
//                 MSG_MAXIMUM: 1,
//                 MSG_MINIMUM: 0,
//                 MSG_TYPE: MSG_NUMBER
//             }
//         },
//           MSG_TYPE: MSG_OBJECT
// };

/**
 * Optional annotations for the client. The client can use annotations to inform how objects are
 * used or displayed
 */
struct FAnnotations
{
	std::optional<std::vector<ERole>> Audience{
		std::nullopt
	}; // Describes who the intended customer of this object or data is. It can include
	   // multiple entries to indicate content useful for multiple audiences (e.g.,
	   // `["user", "assistant"]`).
	std::optional<BoundedDouble> Priority{
		std::nullopt
	}; // 0-1 range. Describes how important this data is for operating the server.
	   // A value of 1 means "most important," and indicates that the data is
	   // effectively required, while 0 means "least important," and
	   // indicates that the data is entirely optional.

	template <typename BasicJSONType> friend void to_json(BasicJSONType& JSON_J, const FAnnotations& JSON_T)
	{
		if (JSON_T.Audience)
		{
			JSON_J["audience"] = JSON_T.Audience.value();
		}
		if (JSON_T.Priority)
		{
			JSON_J["priority"] = JSON_T.Priority.value();
		}
	}

	template <typename BasicJSONType> friend void from_json(const BasicJSONType& JSON_J, FAnnotations& JSON_T)
	{
		if (JSON_J.contains("audience") && JSON_J["audience"].is_array())
		{
			std::vector<ERole> Audience{};
			Audience.reserve(JSON_J["audience"].size());

			for (const auto& AudienceItem : JSON_J["audience"])
			{
				Audience.emplace_back(AudienceItem.template get<ERole>());
			}

			JSON_T.Audience = Audience;
		}
		if (JSON_J.contains("priority"))
		{
			JSON_T.Priority = BoundedDouble::CreateOptional(JSON_J["priority"].template get<double>(), 0.0, 1.0, true);
		}
		else
		{
			JSON_T.Priority = std::nullopt;
		}
	}

	explicit FAnnotations(const std::optional<std::vector<ERole>>& InAudience, const std::optional<double> InPriority)
		: Audience(InAudience),
		  Priority(BoundedDouble::CreateOptional(InPriority, 0.0, 1.0, true))
	{}
	FAnnotations() = default;
};

MCP_NAMESPACE_END