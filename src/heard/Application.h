#include <wrl.h>
#include <WebView2.h>
using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;

class AppLication {
public:
	AppLication(HWND hwnd);
	~AppLication();
	bool InitWebView();
	void Resize(int width, int height);
	HWND GetHwnd() const {
		return hwnd;
	}
private:
	HWND hwnd = nullptr;
	ComPtr<ICoreWebView2Environment> webViewEnvironment;
	ComPtr<ICoreWebView2Controller> webViewController;
	ComPtr<ICoreWebView2> webView;
};