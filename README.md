# win32_webview 🚀

项目概述 🧩
--------

win32_webview 是一个基于 Win32 桌面程序骨架的示例工程，使用本地 WebView（推荐 Microsoft WebView2）作为渲染容器，将现代前端框架（例如 Vue、React）与原生 C++ 程序结合，便于构建混合桌面应用。🖥️🌐

主要特性 ✨
--------

- ✅ 基于 Win32 + C++20 的原生桌面程序骨架
- ✅ 使用 WebView 在本地承载前端页面（推荐 WebView2）
- ✅ 支持 Vue / React 等前端框架开发 UI
- ✅ 使用 CMake + Ninja 构建，兼容 MSVC 工具链 ⚙️

先决条件 🔧
--------

- Windows 10 / 11
- Visual Studio（建议 Visual Studio 2022 / 2026）并安装 C++ 工作负载
- CMake >= 4.3
- Ninja
- WebView2 运行时（用于生产运行）
- Node.js + npm / yarn（用于前端开发与构建）📦

构建与运行 🛠️
------------

在 PowerShell 中（项目根目录）：

- 配置与生成：
  - cmake -S . -B build -G "Ninja"
- 编译：
  - cmake --build build --config Release

生成的可执行文件将位于 build 目录（或 build/Release，取决于 CMake 配置）。📁

前端开发与集成 🌐
----------------

- 前端代码（Vue / React）可放在 frontend/ 或 web/ 目录中。
- 本地开发（热重载）常见两种方式：
  1) 在前端使用 npm run dev 启动开发服务器（例如 http://localhost:3000），在 native 层的 WebView 指向该地址进行调试（便于热更新和快速迭代）。🔁
  2) 生产打包：npm run build，将输出（dist/ 或 build/）复制到 resources/www，程序启动时从本地文件加载。📦➡️📁

示例约定：
- 前端编译产物放到 resources/www 下，运行时 WebView 指向本地文件路径。🗂️

调试提示 🔍
--------

- 如果使用 WebView2，可通过 Edge DevTools 调试前端页面（按需打开 DevTools）。🔧
- 启动时请检查 WebView 是否正确初始化以及资源路径是否正确。确保 WebView2 运行时已安装。⚠️
- 若遇到问题，可在控制台或原生日志中查找错误信息以定位问题（例如资源加载失败、WebView 初始化失败）。🐛

贡献 🤝
----

欢迎提交 issue 与 PR。请保持改动最小化并附带复现步骤或示例代码。📣

许可证 📜
----

本仓库默认未指定具体许可证，请根据需要添加 LICENSE 文件。

感谢使用！🙏

