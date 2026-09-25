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

//void Bridge::SendWebMessage(nlohmann::json& json)
//{
//	std::wstring message = WebViewMessageBridge::AnsiToWString(json.dump(), CP_UTF8);
//	HRESULT hr = this->app.GetWebView()->PostWebMessageAsJson(message.c_str());
//
//	if (FAILED(hr))
//	{
//		wchar_t buffer[64]{};
//
//		swprintf_s(
//			buffer,
//			L"PostWebMessageAsJson failed: 0x%08X",
//			static_cast<unsigned int>(hr)
//		);
//
//		AlertInfo(buffer);
//	}
//}

void Bridge::Handle(WebViewMessageBridge::Message& message)
{
	using namespace WebViewMessageBridge;

	switch (MessageEnumFromString(message.type))
	{
	case MessageEnum::Info:
		AlertInfo(StringToWString(message.value).c_str());
		break;

	case MessageEnum::Http:
	{
		std::thread([this, message]() {
			std::string url = message.value["url"].get<std::string>();
			std::map<std::wstring, std::wstring> headers;
			for (const auto& [name, value] : message.value["headers"].items())
			{
				headers.emplace(StringToWString(name), StringToWString(value.get<std::string>()));
			}

			HttpClient::HttpClient http{};
			if (http.Init(L"", StringToWString(url)))
			{
				http.SetHeaders(headers).Get();

				DWORD status = http.GetStateCode();
				std::string body = http.Send();

				auto* res = new nlohmann::json();
				(*res)["id"] = message.id;
				(*res)["value"] = {
					{"status", status},
					{"body", body}
				};

				PostMessageW(
					this->app.GetHwnd(),
					WM_HTTP_RESPONSE,
					0,
					reinterpret_cast<LPARAM>(res)
				);
			}

			}).detach();
		break;
	}
	}
}