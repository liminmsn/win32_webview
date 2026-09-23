export { };

interface WebView2WebMessageReceivedEventArgs extends Event {
    /**
     * 来自 C++ 宿主环境发送的数据 (JSON 对象或字符串)
     */
    readonly data: any;
    readonly source: WindowProxy;
}

interface WebView2HostObjectsSync {
    [key: string]: any;
}

interface WebView2HostObjects {
    [key: string]: any;
    /**
     * 同步调用 C++ 暴露给前端的对象
     */
    sync: WebView2HostObjectsSync;
}

interface WebView2Core {
    /**
     * 向 C++ WebView2 宿主环境发送消息
     */
    postMessage(message: any): void;

    /**
     * 监听来自 C++ 宿主的 message 事件
     */
    addEventListener(
        type: 'message',
        listener: (event: WebView2WebMessageReceivedEventArgs) => void,
        options?: boolean | AddEventListenerOptions
    ): void;

    /**
     * 移除事件监听
     */
    removeEventListener(
        type: 'message',
        listener: (event: WebView2WebMessageReceivedEventArgs) => void,
        options?: boolean | EventListenerOptions
    ): void;

    /**
     * 映射到 JS 的宿主原生 C++ 对象接口
     */
    hostObjects: WebView2HostObjects;
}

interface ChromeWebView {
    webview: WebView2Core;
}

declare global {
    interface Window {
        chrome?: ChromeWebView;
    }
}