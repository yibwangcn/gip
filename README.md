# Inference Platform

一个基于 C++17 的推理服务平台原型，目标是实现单 HTTP 输入入口、多后端接入、可扩展调度策略的推理服务框架。

当前工程重点放在以下几个方向：

- 模型管理与推理请求生命周期管理
- 面向多策略的调度器设计
- ONNX Runtime 后端接入
- 基于 Pistache 的 HTTP 服务入口

## Current Status

当前项目处于可编译、主流程打通的阶段，整体还在持续演进中。

已具备的能力：

- 基础的模型配置加载与模型管理
- HTTP 推理服务入口
- 调度器分层设计：PolicySelector / Strategy / Scheduler
- FIFO 调度策略
- Batch 策略原型
- ONNX Runtime 后端集成

后续会继续完善：

- 更完整的 batch 调度与 runtime 数据收集
- 更合理的异步执行模型
- metrics / tracing / logging
- 更完整的测试与高并发验证

## Project Structure

```text
include/        Public headers
src/            Core implementation
configs/        Model and service config
docs/           Design notes
tests/          Unit and integration tests
third_party/    Third-party dependencies and CMake glue
```

## Build

当前工程使用 CMake 构建，默认依赖本地目录中的 ONNX Runtime Linux release 包。

```bash
git clone --recurse-submodules https://github.com/yibwangcn/gip.git
cd gip

mkdir -p build
cd build
cmake ..
cmake --build .
```

## Run

当前入口位于 `src/main.cpp`，启动时会读取 `configs/` 目录下的配置。

```bash
./src/inference_platform
```

如果你的构建产物路径不同，可以在 `build/` 目录下查找实际生成的可执行文件。

## Dependencies

项目当前显式使用了以下第三方库：

- ONNX Runtime: <https://github.com/microsoft/onnxruntime>
- nlohmann/json: <https://github.com/nlohmann/json>
- Pistache: <https://github.com/pistacheio/pistache>

其中：

- `third_party/json` 和 `third_party/pistache` 以 git submodule 形式管理
- ONNX Runtime 当前使用本地下载包方式接入，默认路径为 `onnxruntime-linux-x64-1.24.4/`

## Notes

如果你在 clone 后发现 submodule 目录为空，请执行：

```bash
git submodule update --init --recursive
```
