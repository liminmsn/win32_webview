#include "heard/Bridge.h"
#include "heard/AppLication.h"
#include <thread>
#include "../core/Window.hpp"
#include "../core/MessageBox.hpp"

Bridge::Bridge(ComPtr<ICoreWebView2>& webview, AppLication& app) :webview(webview), app(app) {
	static WebViewMessageBridge::WebViewMessageBridge bridgeMessage;
	bridgeMessage.SetMessageHandler(
		[this](WebViewMessageBridge::Message& message) {
			Handle(message);
		});
	webview->add_WebMessageReceived(
		Callback<ICoreWebView2WebMessageReceivedEventHandler>(
			[this](ICoreWebView2*, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
				using nlohmann::json;

				LPWSTR message = nullptr;
				HRESULT hr = args->get_WebMessageAsJson(&message);
				if (SUCCEEDED(hr) && message) {
					try {
						bridgeMessage.Send(message);
					}
					catch (const nlohmann::json::exception& e) {
						OutputDebugStringA(e.what());
					}
					CoTaskMemFree(message);
				}
				return S_OK;
			}
		).Get(),
		nullptr
	);
}

void Bridge::Handle(WebViewMessageBridge::Message& message)
{
	using namespace WebViewMessageBridge;
	HWND hwnd = this->app.GetHwnd();
	switch (MessageEnumFromString(message.type))
	{
	case MessageEnum::Unknown:
		AlertInfo(StringToWString(message.value).c_str());
		break;
	case MessageEnum::Http:
	{
		std::thread([this, message, hwnd]() {
			std::string url = message.value["url"].get<std::string>();
			std::string method = message.value["method"].get<std::string>();

			std::map<std::wstring, std::wstring> headers;
			for (const auto& [name, value] : message.value["headers"].items())
			{
				headers.emplace(StringToWString(name), StringToWString(value.get<std::string>()));
			}

			HttpClient::HttpClient http{};

			if (http.Init(L"localhost", StringToWString(url)))
			{
				if (message.value.contains("body")) {
					const std::string body = message.value["body"].get<std::string>();
					http.Request(StringToWString(method), body);
				}
				else {
					http.Request(StringToWString(method));
				}

				DWORD status = http.GetStateCode();
				std::string body = http.Send();

				auto* res = new nlohmann::json();
				(*res)["id"] = message.id;
				(*res)["value"] = {
					{"status", status},
					{"body", body}
				};

				PostMessageW(hwnd, WM_WEBVIEW_RESPONSE, 0, reinterpret_cast<LPARAM>(res));
			}
			}
		).detach();
		break;
	}
	case MessageEnum::Client:
	{

		const std::string type = message.value["type"].get<std::string>();
		if (type == "IsFullscreen") {
			const bool res = HwndControl::IsFullscreen(hwnd);

			PostMessageW(
				hwnd,
				WM_WEBVIEW_RESPONSE,
				static_cast<WPARAM>(res),
				0
			);

			return;
		}

		HwndControl::Call(hwnd, type.c_str());
		break;
	}
	}
}