#pragma once

#include <string>
#include <wrl.h>
#include <WebView2.h>
#include <nlohmann/json.hpp>

using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;

enum class MessageEnum { Info, Warning, Error, Debug };
inline static std::string ToString(MessageEnum type) {
	switch (type) {
	case MessageEnum::Info:    return "info";
	case MessageEnum::Warning: return "warning";
	case MessageEnum::Error:   return "error";
	case MessageEnum::Debug:   return "debug";
	default:                   return "unknown";
	}
}

struct MessageData { std::string message; };
struct Message { std::string type;	MessageData data; };
inline void from_json(const nlohmann::json& json, MessageData& data) {
	data.message = json.value("message", "");
}

inline void from_json(const nlohmann::json& json, Message& message) {
	message.type = json.value("type", "");
	message.data = json.value("data", MessageData{});
}


class AppLication;
class WebMessage {
public:
	explicit WebMessage(ComPtr<ICoreWebView2>& webview, AppLication& app);

private:
	const AppLication& app;
	static std::string WideToUtf8(const wchar_t* value);
	void Handle(Message& message);
};