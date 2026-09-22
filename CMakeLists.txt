cmake_minimum_required(VERSION 4.3)

project(win_pktmon LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------- 2. 全局编译选项 ----------
if(MSVC)
    add_compile_options(/utf-8 /W4 /permissive-)
endif()

find_package(nlohmann_json CONFIG REQUIRED)
find_package(unofficial-webview2 CONFIG REQUIRED)

file(GLOB_RECURSE SRC_FILES  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_executable(
    win_pktmon
    WIN32
    resources/resource.rc
    main.cpp
    global.h
    ${SRC_FILES}
)

target_include_directories(win_pktmon PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(win_pktmon PRIVATE
    nlohmann_json::nlohmann_json
    unofficial::webview2::webview2
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:APP_DEVELOPMENT>
    $<$<CONFIG:Release>:APP_RELEASE>
)