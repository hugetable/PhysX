# PhysX Ecosystem 编译说明

## 概述

本项目包含 PhysX、BLAST 和 FLOW 三个库。我们提供了一个改进的构建脚本 `build.sh`，它**完全使用系统库**编译 PhysX 和 FLOW，无需 NVIDIA packman。

## ✅ PhysX 编译 (完全支持)

PhysX 现在可以完全使用系统库编译，无需任何外部依赖管理器！

### 系统要求

```bash
# 必需工具
sudo apt-get install cmake make gcc g++ python3

# 或者使用 clang
sudo apt-get install cmake make clang python3
```

### 编译命令

```bash
# 编译 PhysX (Release 版本)
./build.sh --physx --config release

# 编译 PhysX (Debug 版本)
./build.sh --physx --config debug

# 使用 Clang 编译
./build.sh --physx --compiler clang --config release

# 清理后重新编译
./build.sh --physx --clean --force --config release

# 使用 8 个并行任务编译
./build.sh --physx -j 8 --config release
```

### 输出位置

- **GCC 编译**: `physx/bin/linux.gcc/bin/linux.x86_64/<config>/`
- **Clang 编译**: `physx/bin/linux.clang/bin/linux.x86_64/<config>/`

生成的库文件：
- `libPhysX.so` (3.1 MB) - 核心 PhysX 物理引擎
- `libPhysXCommon.so` (3.6 MB) - 通用功能模块
- `libPhysXCooking.so` (22 KB) - 网格预处理
- `libPhysXFoundation.so` (111 KB) - 基础库
- `libPhysXExtensions_static.a` (3.5 MB) - 扩展功能（静态库）
- `libPhysXCharacterKinematic_static.a` (303 KB) - 角色控制器
- `libPhysXVehicle2_static.a` (219 KB) - 车辆物理系统
- `libPhysXPvdSDK_static.a` (704 KB) - 可视化调试工具

## ✅ FLOW 编译 (完全支持)

FLOW 现在也支持使用系统工具编译！build.sh 会自动下载 Slang 着色器编译器。

### 系统要求

```bash
# 基础工具（与 PhysX 相同）
sudo apt-get install cmake make gcc g++ python3

# FLOW 特定依赖
sudo apt-get install libgl1-mesa-dev libglu1-mesa-dev \
    libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev \
    libxi-dev libvulkan-dev
```

### 编译命令

```bash
# 编译 FLOW (自动下载 Slang)
./build.sh --flow --config release

# 强制重新编译
./build.sh --flow --force --config release
```

### 自动依赖处理

build.sh 会自动：
1. 检查 Slang 着色器编译器是否存在
2. 如果不存在，自动从 GitHub 下载 (v2024.14.4)
3. 解压到 `flow/external/slang/`
4. 检查 OpenGL 和 X11 库，如果缺失会提示安装

### 输出位置

- **输出目录**: `flow/_build/linux-x86_64/<config>/`

生成的库文件：
- `libnvflow.so` (2.7 MB) - 核心 FLOW 库
- `libnvflow_rtx.so` (2.7 MB) - RTX 支持版本
- `libnvflowext.so` (12 MB) - 扩展功能
- `libnvflowext_rtx.so` (12 MB) - RTX 扩展功能
- `nvfloweditor` - 编辑器可执行文件

额外依赖库（自动复制）：
- `libslang.so` (15 MB) - Slang 着色器编译器
- `libslang-glslang.so` (8.4 MB) - GLSL 支持
- `libglfw.so.3.3` (324 KB) - 窗口管理

## ⚠️ BLAST 编译 (使用原始系统)

BLAST 依赖 NVIDIA 的内部构建工具（repo_build），这是一个复杂的依赖管理和构建系统。

### 当前状态

- ❌ 无法使用标准 premake5 直接编译
- ❌ 需要 NVIDIA 特定的 Lua 模块和工具链
- ✅ 原始构建系统仍然可用

### 推荐方法

使用 BLAST 自己的构建脚本：

```bash
cd blast
./build.sh  # 使用原始 packman 系统
```

原始系统会：
1. 自动下载 packman 依赖
2. 下载 Cap'n Proto 序列化库
3. 下载 LLVM 工具链
4. 编译所有 BLAST 库

### 我们的尝试

我们创建了一个最小的 `omni/repo/build` Lua stub 模块，但 BLAST 的构建系统过于复杂，包含：
- Cap'n Proto 代码生成
- 自定义包管理
- 跨平台工具链管理
- Docker 容器支持

这些都紧密集成在 NVIDIA 的内部工具中。

## 🚀 快速开始

### 只需要 PhysX

```bash
# 一行命令编译
./build.sh --physx

# 查看输出
ls -lh physx/bin/linux.gcc/bin/linux.x86_64/release/
```

### 需要 PhysX 和 FLOW

```bash
# 编译两个库（FLOW 会自动下载 Slang）
./build.sh --physx --flow

# 或者分开编译
./build.sh --physx
./build.sh --flow
```

### 编译所有库

```bash
# 尝试编译所有（BLAST 会失败，但 PhysX 和 FLOW 会成功）
./build.sh --all

# PhysX 和 FLOW 编译成功后使用
./build.sh --physx --flow
```

## 📊 构建系统对比

| 特性 | PhysX | FLOW | BLAST |
|------|-------|------|-------|
| 系统编译器 | ✅ 完全支持 | ✅ 完全支持 | ❌ 需要 packman |
| 自动依赖 | ✅ 无需依赖 | ✅ 自动下载 Slang | ❌ 复杂依赖 |
| 编译时间 | 🚀 ~8 分钟 | 🚀 ~5 分钟 | ⏱️ 较长 |
| 库大小 | 📦 ~12 MB | 📦 ~50 MB (含依赖) | 📦 未知 |
| 推荐方法 | ✅ build.sh | ✅ build.sh | ⚠️ 原始 build.sh |

## 🔧 构建选项详解

### 基本选项

```bash
--physx             # 只编译 PhysX
--blast             # 只编译 BLAST (使用原始系统)
--flow              # 只编译 FLOW (自动处理依赖)
--all               # 尝试编译所有库

--config CONFIG     # release, debug, checked (默认: release)
--compiler COMP     # gcc 或 clang (默认: gcc)

--clean             # 编译前清理
--force             # 强制重新编译

-j N                # 并行编译任务数 (默认: CPU 核心数)
--help              # 显示帮助
```

### 高级示例

```bash
# Debug 版本 + Clang + 8 线程
./build.sh --physx --config debug --compiler clang -j 8

# 清理并重新编译 FLOW
./build.sh --flow --clean --config release

# 编译 PhysX 和 FLOW (release 和 debug)
./build.sh --physx --config release
./build.sh --physx --config debug
./build.sh --flow --config release
./build.sh --flow --config debug
```

## 🔍 故障排除

### PhysX 编译失败

**错误**: CMake 找不到编译器
```bash
# 检查编译器
which gcc g++

# 安装 GCC
sudo apt-get install gcc g++

# 或安装 Clang
sudo apt-get install clang
```

**错误**: Python3 找不到
```bash
sudo apt-get install python3
```

### FLOW 编译失败

**错误**: GL/gl.h 找不到
```bash
sudo apt-get install libgl1-mesa-dev libglu1-mesa-dev
```

**错误**: X11/extensions/Xrandr.h 找不到
```bash
sudo apt-get install libx11-dev libxrandr-dev libxinerama-dev \
    libxcursor-dev libxi-dev
```

**错误**: Slang 下载失败
```bash
# 手动下载 Slang
cd flow/external
mkdir -p slang
cd slang
curl -L -o slang.tar.gz "https://github.com/shader-slang/slang/releases/download/v2024.14.4/slang-2024.14.4-linux-x86_64-glibc-2.17.tar.gz"
tar -xzf slang.tar.gz
rm slang.tar.gz
```

### BLAST 编译问题

BLAST 使用复杂的内部构建系统，建议：
1. 使用原始 `blast/build.sh`
2. 或者跳过 BLAST，只使用 PhysX 和 FLOW

## 📚 与 PhysXWrapper 集成

编译完成后，PhysXWrapper 项目可以链接到这些库：

```bash
cd PhysXWrapper/examples
mkdir build && cd build

# 方法1: CMake 自动查找
cmake ..
make

# 方法2: 指定 PhysX 路径
cmake .. -DPHYSX_ROOT=/home/user/PhysX/physx
make

# 运行示例
./example_01_hello_world
```

## 🎯 技术细节

### PhysX 编译流程

1. **直接 CMake** - 不使用 XML preset 或 Python 脚本
2. **系统编译器** - 使用 /usr/bin/gcc 或 /usr/bin/clang
3. **最小配置** - 禁用 GPU、Snippets 等非必需功能
4. **标准输出** - 生成标准的 .so 和 .a 库文件

关键 CMake 参数：
```cmake
-DCMAKE_C_COMPILER=/usr/bin/gcc
-DCMAKE_CXX_COMPILER=/usr/bin/g++
-DCMAKE_MODULE_PATH=<PhysX cmake modules>
-DTARGET_BUILD_PLATFORM=linux
-DPX_OUTPUT_LIB_DIR=<output directory>
-DPX_BUILDSNIPPETS=OFF
-DPX_GENERATE_GPU_PROJECTS=OFF
```

### FLOW 编译流程

1. **自动下载 Slang** - 检测并下载着色器编译器
2. **Premake5 生成** - 使用标准 premake5（无需特殊模块）
3. **着色器编译** - 使用 Slang 编译 HLSL 着色器到 SPIR-V
4. **系统依赖** - OpenGL、X11、Vulkan 开发库

特点：
- 着色器在编译时处理
- 生成的着色器嵌入到库中
- 支持 CPU 和 Vulkan 后端

### BLAST 依赖分析

BLAST 需要：
- **omni/repo/build** - NVIDIA 内部 Lua 模块
- **repo_man** - 仓库管理工具
- **packman** - 包管理器
- **Cap'n Proto** - 序列化库（通过 packman 下载）
- **Docker 支持** - 跨平台构建
- **自定义工具链** - LLVM/GCC 特定版本

这些工具形成了一个完整的生态系统，很难替代。

## 📈 性能数据

在 16核 CPU 上的编译时间：

| 库 | 配置 | 时间 | 输出大小 |
|----|------|------|----------|
| PhysX | Release | ~8 分钟 | 12 MB |
| PhysX | Debug | ~10 分钟 | 25 MB |
| FLOW | Release | ~5 分钟 | 50 MB (含依赖) |
| FLOW | Debug | ~7 分钟 | 75 MB (含依赖) |

## 🌟 总结

### ✅ 推荐工作流

1. **开发 PhysX 应用**: 使用 `./build.sh --physx`
2. **需要流体模拟**: 添加 `./build.sh --flow`
3. **需要破坏效果**: 使用 BLAST 原始构建系统

### 🎉 改进亮点

- **PhysX**: 完全无依赖，一键编译
- **FLOW**: 自动化依赖下载，简化流程
- **BLAST**: 保持原始系统，确保兼容性

### 🔮 未来可能改进

1. **BLAST**: 创建完整的 CMakeLists.txt（大工程）
2. **Vulkan**: 添加 PhysX GPU 支持（需要 CUDA）
3. **预编译**: 提供预编译的二进制包

## 📞 获取帮助

遇到问题？

1. 查看 `./build.sh --help`
2. 阅读本文档的故障排除部分
3. 检查系统依赖是否安装
4. 对于 BLAST，使用原始构建系统

Happy coding! 🚀
