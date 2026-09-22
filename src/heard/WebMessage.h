#pragma once

#include <string>
#include <wrl.h>
#include <WebView2.h>
#include <nlohmann/json.hpp>

using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;

class AppLication;

struct MessageData {
	std::string message;
};

struct Message {
	std::string type;
	MessageData data;
};

inline void from_json(const nlohmann::json& json, MessageData& data) {
	data.message = json.value("message", "");
}

inline void from_json(const nlohmann::json& json, Message& message) {
	message.type = json.value("type", "");
	message.data = json.value("data", MessageData{});
}

class WebMessage {
public:
	explicit WebMessage(ComPtr<ICoreWebView2>& webview, AppLication& app);

private:
	const AppLication& app;

	static std::string WideToUtf8(const wchar_t* value);
};