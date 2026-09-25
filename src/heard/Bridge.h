#pragma once
#include <string>
#include <wrl.h>
#include <WebView2.h>
#include <nlohmann/json.hpp>
#include <webViewMessageBredge/WebViewMessageBridge.hpp>
#include <httpClient/HttpClient.hpp>
using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;

class AppLication;
class Bridge {
public:
	explicit Bridge(ComPtr<ICoreWebView2>& webview, AppLication& app);
	void Handle(WebViewMessageBridge::Message& message);
private:
	const ComPtr<ICoreWebView2>& webview;
	const AppLication& app;
};