# Joker-Player

一款能添加 VST 插件的播放器，旨在不打开 DAW 就能快速测试插件。

能稳定加载插件进行处理，能调用 ASIO。

![Joker Player](Joker%20Player.png)

## 功能

- 加载 VST2 / VST3 插件，对正在播放的音频进行实时处理
- 支持 ASIO 声卡输出
- 播放列表管理，支持常见音频格式（wav / aiff / flac / mp3 等）

## 路由说明

- 插入1 为主处理通道
- 插入2 为平行处理通道
- 发送通道输入插入1 的信号
- 3 条链路合并输入总线通道
- 总线上的插件具有记忆性

## 构建（Windows）

仓库内已包含 JUCE、ASIO SDK 和 VST2 SDK，无需单独下载。

环境要求：

- CMake 3.22 及以上
- 支持 C++17 的编译器（如 Visual Studio 2019 / 2022）

```bash
git clone https://github.com/Cannon-p/Joker-Player.git
cd Joker-Player
cmake -B build
cmake --build build --config Release
```

构建完成后，可执行文件 `Joker Player.exe` 位于构建目录中。

## 目录结构

```text
CMakeLists.txt              顶层构建脚本
src/                        源代码（播放引擎、插件管理、界面）
lib/JUCE/                   JUCE 框架
asiosdk_2.3.3_2019-06-14/   Steinberg ASIO SDK
vst2_sdk/                   VST2 SDK 头文件
Joker Player.png            界面截图
LICENSE                     MIT 许可证
```

## 许可证

本项目基于 [MIT License](LICENSE) 开源。
