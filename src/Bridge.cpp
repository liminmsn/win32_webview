#include "../core/ProcessConnections.hpp"
#include "heard/Bridge.h"
#include "heard/AppLication.h"
#include "../core/ProcessPopup.hpp"
#include "../core/MessageBox.hpp"
#include <thread>

std::string Bridge::WideToUtf8(const wchar_t* value) {
	if (!value)
		return {};

	int size = WideCharToMultiByte(CP_UTF8, 0, value, -1, nullptr, 0, nullptr, nullptr);
	if (size <= 0)
		return {};

	std::string result(size - 1, '\0');
	WideCharToMultiByte(CP_UTF8, 0, value, -1, result.data(), size, nullptr, nullptr);
	return result;
}

Bridge::Bridge(ComPtr<ICoreWebView2>& webview, AppLication& app) : app(app) {
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

void Bridge::Handle(Message& message) {
	switch (MessageEnumFromString(message.type)) {
	case MessageEnum::Info:
		AlertInfo(StringToWString(message.data.message).c_str());
		break;
	case MessageEnum::Client:
		ProcessInfo process = ProcessPopup::Show(this->app.GetHwnd());
		if (process.pid != 0)
		{
			auto connections = GetProcessConnections(process.pid);
			std::wstring text = FormatConnections(connections);

			std::wstring title = L"进程连接信息 - PID " + std::to_wstring(process.pid);
			MessageBoxW(this->app.GetHwnd(), text.c_str(), title.c_str(),
				MB_OK | MB_ICONINFORMATION);
		}
		break;
	}
	//std::thread([this, message]()
	//	{
	//		
	//	}
	//).detach();
}