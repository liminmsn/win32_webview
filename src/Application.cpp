#include "heard/AppLication.h"
#include "heard/WebMessage.h"
#include <iostream>

std::unique_ptr<WebMessage> webMessage;
AppLication::AppLication(HWND hwnd) : hwnd(hwnd) {}
AppLication::~AppLication() {
	if (webViewController) {
		webViewController->Close();
	}
}

void AppLication::Resize(int width, int height) {
	if (!webViewController) return;
	RECT bounds{ 0, 0, width, height };
	webViewController->put_Bounds(bounds);
}

bool AppLication::InitWebView() {
	HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr,
		Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
			[this](HRESULT result, ICoreWebView2Environment* environment) -> HRESULT {
				if (FAILED(result) || !environment)
					return result;

				webViewEnvironment = environment;

				return webViewEnvironment->CreateCoreWebView2Controller(hwnd,
					Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
						[this](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
							if (FAILED(result) || !controller)
								return result;

							webViewController = controller;

							ComPtr<ICoreWebView2Controller2> controller2;
							HRESULT hr = webViewController.As(&controller2);
							if (FAILED(hr))
								return hr;

							COREWEBVIEW2_COLOR color{ 0,0,0,0 };
							controller2->put_DefaultBackgroundColor(color);

							RECT bounds{};
							GetClientRect(hwnd, &bounds);
							webViewController->put_Bounds(bounds);

							hr = webViewController->get_CoreWebView2(&webView);
							if (FAILED(hr))
								return hr;

							webMessage = std::make_unique<WebMessage>(this->webView, *this);
#if APP_DEVELOPMENT
							webView->Navigate(L"http://localhost:5173/");
#elif APP_RELEASE
							webView->Navigate(L"https://www.baidu.com");
#endif
							return S_OK;
						}).Get());
			}).Get());
	return SUCCEEDED(hr);
}