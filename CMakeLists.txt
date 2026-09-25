cmake_minimum_required(VERSION 4.3)

project(win32_webview LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------- 2. 全局编译选项 ----------
if(MSVC)
    add_compile_options(/utf-8 /W4 /permissive-)
endif()

find_package(nlohmann_json CONFIG REQUIRED)
find_package(unofficial-webview2 CONFIG REQUIRED)

add_subdirectory(
    E:/source/HttpClient
    ${CMAKE_CURRENT_BINARY_DIR}/HttpClient
)
add_subdirectory(
    E:/source/WebViewMessageBridge
    ${CMAKE_CURRENT_BINARY_DIR}/WebViewMessageBridge
)

file(GLOB_RECURSE SRC_FILES  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_executable(
    win32_webview
    WIN32
    resources/resource.rc
    main.cpp
    global.h
    ${SRC_FILES}
)

target_include_directories(win32_webview PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(win32_webview PRIVATE
    nlohmann_json::nlohmann_json
    unofficial::webview2::webview2
    WebViewMessageBridge
    HttpClient
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:APP_DEVELOPMENT>
    $<$<CONFIG:Release>:APP_RELEASE>
)