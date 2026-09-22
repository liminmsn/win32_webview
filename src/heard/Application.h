#include <wrl.h>
#include <WebView2.h>

class AppLication {
public:
	AppLication(HWND hwnd);
	~AppLication();
	bool InitWebView();
	void Resize(int width, int height);
private:
	HWND hwnd = nullptr;
	Microsoft::WRL::ComPtr<ICoreWebView2Environment> webViewEnvironment;
	Microsoft::WRL::ComPtr<ICoreWebView2Controller> webViewController;
	Microsoft::WRL::ComPtr<ICoreWebView2> webView;
};