#pragma once

#include <Poco/Net/MediaType.h>
#include <Poco/URI.h>

#include <optional>
#include <vector>

#include "CoreSDK/Common/BaseTypes.h"
#include "URIProxy.h"
#include "json.hpp"

template <typename T> struct nlohmann::adl_serializer<std::vector<T>>
{
	static void to_json(json& j, const std::vector<T>& vec)
	{
		j = json::array();
		for (const auto& item : vec)
		{
			j.emplace_back(item);
		}
	}

	static void from_json(const json& j, std::vector<T>& vec)
	{
		if (j.is_array())
		{
			vec.clear();
			vec.reserve(j.size());
			for (const auto& item : j)
			{
				vec.emplace_back(item.get<T>());
			}
		}
		else
		{
			vec.clear();
		}
	}
};

template <> struct nlohmann::adl_serializer<BoundedDouble>
{
	template <typename BasicJsonType> static void to_json(BasicJsonType& json_value, const BoundedDouble& dbl)
	{
		json_value = dbl.GetValue();
	}

	template <typename BasicJsonType> static void from_json(const BasicJsonType& json_value, BoundedDouble& dbl)
	{
		const auto& val = json_value.template get<double>();
		dbl = BoundedDouble(val);
	}
};

template <typename T> struct nlohmann::adl_serializer<std::unique_ptr<T>>
{
	template <typename BasicJsonType> static void to_json(BasicJsonType& json_value, const std::unique_ptr<T>& ptr)
	{
		if (ptr.get())
		{
			json_value = *ptr;
		}
		else
		{
			json_value = nullptr;
		}
	}

	template <typename BasicJsonType> static void from_json(const BasicJsonType& json_value, std::unique_ptr<T>& ptr)
	{
		T inner_val = json_value.template get<T>();
		ptr = std::make_unique<T>(std::move(inner_val));
	}
};

template <typename T> struct nlohmann::adl_serializer<std::optional<T>>
{
	static void to_json(json& j, const std::optional<T>& opt)
	{
		if (opt)
		{
			j = opt.value();
		}
		else
		{
			j = nullptr;
		}
	}

	static void from_json(const json& j, std::optional<T>& opt)
	{
		if (j.is_null())
		{
			opt = std::nullopt;
		}
		else
		{
			opt = j.get<T>();
		}
	}
};

template <> struct nlohmann::adl_serializer<Poco::URI>
{
	static void to_json(json& j, const Poco::URI& uri) { j = uri.toString(); }

	static void from_json(const json& j, Poco::URI& uri)
	{
		if (j.is_string())
		{
			uri = Poco::URI(j.get<std::string>());
		}
	}
};

template <> struct nlohmann::adl_serializer<MCP::FMIMEType>
{
	static void to_json(json& j, const MCP::FMIMEType& mediaType) { j = mediaType.toString(); }

	static void from_json(const json& j, MCP::FMIMEType& mediaType)
	{
		if (j.is_string())
		{
			mediaType = MCP::FMIMEType(j.get<std::string>());
		}
	}
};
