#pragma once
#include <wrl.h>
#include <WebView2.h>
#include <nlohmann/json.hpp>

using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;

class AppLication {
public:
	AppLication(HWND hwnd);
	~AppLication();
	bool InitWebView();
	void Resize(int width, int height);
	void SendWebMessage(nlohmann::json& json);
	HWND GetHwnd() const {
		return hwnd;
	}
	ComPtr<ICoreWebView2> GetWebView() const {
		return webView;
	}
private:
	HWND hwnd = nullptr;
	ComPtr<ICoreWebView2Environment> webViewEnvironment;
	ComPtr<ICoreWebView2Controller> webViewController;
	ComPtr<ICoreWebView2> webView;
};