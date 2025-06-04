#pragma once

#include <optional>
#include <string>

#include "../../../ThirdParty/json.hpp"

namespace MCP {

using JSON = nlohmann::json;

class MessageBase {
  public:
    virtual ~MessageBase() = default;
    virtual std::string GetJSONRPCVersion() const = 0;
    virtual std::optional<std::string> GetID() const = 0;
    virtual std::optional<std::string> GetMethod() const = 0;
    virtual std::optional<JSON> GetParams() const = 0;
    virtual std::optional<JSON> GetResult() const = 0;
    virtual std::optional<JSON> GetError() const = 0;
    virtual JSON ToJSON() const = 0;
};

class RequestMessage : public MessageBase {
  public:
    RequestMessage(const std::string& id, const std::string& method,
                   const JSON& params = JSON::object())
        : id_(id), method_(method), params_(params) {}

    std::string GetJSONRPCVersion() const override {
        return "2.0";
    }
    std::optional<std::string> GetID() const override {
        return id_;
    }
    std::optional<std::string> GetMethod() const override {
        return method_;
    }
    std::optional<JSON> GetParams() const override {
        return std::make_optional(params_);
    }
    std::optional<JSON> GetResult() const override {
        return std::nullopt;
    }
    std::optional<JSON> GetError() const override {
        return std::nullopt;
    }

    JSON ToJSON() const override {
        JSON j;
        j["jsonrpc"] = GetJSONRPCVersion();
        j["id"] = id_;
        j["method"] = method_;
        j["params"] = params_;
        return j;
    }

  private:
    std::string id_;
    std::string method_;
    JSON params_;
};

class ResponseMessage : public MessageBase {
  public:
    ResponseMessage(const std::string& id, const JSON& result) : id_(id), result_(result) {}

    std::string GetJSONRPCVersion() const override {
        return "2.0";
    }
    std::optional<std::string> GetID() const override {
        return id_;
    }
    std::optional<std::string> GetMethod() const override {
        return std::nullopt;
    }
    std::optional<JSON> GetParams() const override {
        return std::nullopt;
    }
    std::optional<JSON> GetResult() const override {
        return std::make_optional(result_);
    }
    std::optional<JSON> GetError() const override {
        return std::nullopt;
    }

    JSON ToJSON() const override {
        JSON j;
        j["jsonrpc"] = GetJSONRPCVersion();
        j["id"] = id_;
        j["result"] = result_;
        return j;
    }

  private:
    std::string id_;
    JSON result_;
};

class ErrorMessage : public MessageBase {
  public:
    ErrorMessage(const std::string& id, const JSON& error) : id_(id), error_(error) {}

    std::string GetJSONRPCVersion() const override {
        return "2.0";
    }
    std::optional<std::string> GetID() const override {
        return id_;
    }
    std::optional<std::string> GetMethod() const override {
        return std::nullopt;
    }
    std::optional<JSON> GetParams() const override {
        return std::nullopt;
    }
    std::optional<JSON> GetResult() const override {
        return std::nullopt;
    }
    std::optional<JSON> GetError() const override {
        return std::make_optional(error_);
    }

    JSON ToJSON() const override {
        JSON j;
        j["jsonrpc"] = GetJSONRPCVersion();
        j["id"] = id_;
        j["error"] = error_;
        return j;
    }

  private:
    std::string id_;
    JSON error_;
};

class NotificationMessage : public MessageBase {
  public:
    NotificationMessage(const std::string& method, const JSON& params = JSON::object())
        : method_(method), params_(params) {}

    std::string GetJSONRPCVersion() const override {
        return "2.0";
    }
    std::optional<std::string> GetID() const override {
        return std::nullopt;
    }
    std::optional<std::string> GetMethod() const override {
        return method_;
    }
    std::optional<JSON> GetParams() const override {
        return std::make_optional(params_);
    }
    std::optional<JSON> GetResult() const override {
        return std::nullopt;
    }
    std::optional<JSON> GetError() const override {
        return std::nullopt;
    }

    JSON ToJSON() const override {
        JSON j;
        j["jsonrpc"] = GetJSONRPCVersion();
        j["method"] = method_;
        j["params"] = params_;
        return j;
    }

  private:
    std::string method_;
    JSON params_;
};

} // namespace MCP