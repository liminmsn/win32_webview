#include "heard/WebMessage.h"
#include "heard/AppLication.h"
#include "../core/MessageBox.hpp"
#include <thread>

std::string WebMessage::WideToUtf8(const wchar_t* value) {
	if (!value)
		return {};

	int size = WideCharToMultiByte(CP_UTF8, 0, value, -1, nullptr, 0, nullptr, nullptr);
	if (size <= 0)
		return {};

	std::string result(size - 1, '\0');
	WideCharToMultiByte(CP_UTF8, 0, value, -1, result.data(), size, nullptr, nullptr);
	return result;
}

WebMessage::WebMessage(ComPtr<ICoreWebView2>& webview, AppLication& app) : app(app) {
	webview->add_WebMessageReceived(
		Callback<ICoreWebView2WebMessageReceivedEventHandler>(
			[this](ICoreWebView2*, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
				LPWSTR message = nullptr;

				HRESULT hr = args->get_WebMessageAsJson(&message);

				if (SUCCEEDED(hr) && message) {
					try {
						std::string jsonString = WideToUtf8(message);
						nlohmann::json json = nlohmann::json::parse(jsonString);
						Message message = json.get<Message>();
						this->Handle(message);
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

void WebMessage::Handle(Message& message) {
	std::thread([this, message]()
		{
			if (ToString(MessageEnum::Info) == message.type) {
				AlertInfo(StringToWString(message.data.message).c_str());
			}
		}
	).detach();
}