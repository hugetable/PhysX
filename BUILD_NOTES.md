# PhysX Ecosystem 编译说明

## 概述

本项目包含 PhysX、BLAST 和 FLOW 三个库。为了简化编译流程，我们提供了一个统一的构建脚本 `build.sh`，它使用 **系统库** 而不是 NVIDIA packman 来编译这些库。

## ✅ PhysX 编译 (已完成)

PhysX 现在可以完全使用系统库编译，无需 packman！

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
```

### 输出位置

- **GCC 编译**: `physx/bin/linux.gcc/<config>/`
- **Clang 编译**: `physx/bin/linux.clang/<config>/`

生成的库文件：
- `libPhysX.so` - 核心 PhysX 库
- `libPhysXCommon.so` - 通用功能
- `libPhysXCooking.so` - 网格处理
- `libPhysXFoundation.so` - 基础库
- `libPhysXExtensions_static.a` - 扩展功能
- `libPhysXCharacterKinematic_static.a` - 角色控制器
- `libPhysXVehicle2_static.a` - 车辆系统

## ⚠️ BLAST 编译 (需要额外依赖)

BLAST 使用 premake5 + Lua 模块构建系统，依赖于 NVIDIA 的内部构建工具。

### 当前状态

- ❌ 直接使用标准 premake5 会失败 (缺少 `omni/repo/build` 模块)
- ❌ 没有现成的 CMakeLists.txt

### 可能的解决方案

1. **使用 NVIDIA 的 packman 系统** (原始方法)
   ```bash
   cd blast
   ./build.sh  # 使用原始脚本
   ```

2. **手动创建 CMakeLists.txt** (需要大量工作)
   - 需要分析 premake5.lua 配置
   - 手动创建 CMake 构建文件

## ⚠️ FLOW 编译 (需要额外依赖)

FLOW 也使用 premake5 构建系统，并且依赖于 Slang 着色器编译器。

### 当前状态

- ⚠️ premake5 生成成功
- ❌ 编译失败：缺少 `external/slang/lib/libslang.so`

### 缺少的依赖

1. **Slang 着色器编译器**
   - FLOW 需要 Slang 来编译着色器
   - 通常由 packman 下载
   - 可以从 https://github.com/shader-slang/slang 获取

### 可能的解决方案

1. **手动安装 Slang**
   ```bash
   # 下载 Slang
   cd flow/external
   mkdir -p slang/lib
   # 从 https://github.com/shader-slang/slang/releases 下载
   # 解压并复制 libslang.so 到 slang/lib/
   ```

2. **使用 NVIDIA 的 packman 系统** (原始方法)
   ```bash
   cd flow
   ./build.sh  # 使用原始脚本
   ```

## 构建脚本使用说明

### 基本用法

```bash
# 构建所有库 (会尝试编译 PhysX, BLAST, FLOW)
./build.sh --all

# 只构建 PhysX
./build.sh --physx

# 构建 PhysX 和 BLAST
./build.sh --physx --blast

# 查看帮助
./build.sh --help
```

### 配置选项

```bash
# 指定配置 (release, debug, checked)
./build.sh --physx --config release

# 指定编译器 (gcc, clang)
./build.sh --physx --compiler clang

# 指定并行编译数
./build.sh --physx -j 8

# 清理后编译
./build.sh --physx --clean

# 强制重新编译
./build.sh --physx --force
```

## 编译成功示例

```bash
$ ./build.sh --physx --config release

========================================
Build Configuration
========================================
Libraries:
  - PhysX
Config: release
Compiler: gcc
Jobs: 16

========================================
Building PhysX (release) with system gcc
========================================
ℹ Using system gcc compiler
ℹ Build configuration: release
ℹ C Compiler: /usr/bin/gcc
ℹ C++ Compiler: /usr/bin/g++
ℹ Generating PhysX build files with CMake...

[编译过程...]

✓ PhysX (release) built successfully with system gcc
ℹ Libraries location: physx/bin/linux.gcc/release/

========================================
Build Summary
========================================
✓ All builds completed successfully!

Library locations:
  PhysX: physx/bin/linux.gcc/release/
```

## 与 PhysXWrapper 的集成

编译完成后，PhysXWrapper 项目可以链接到这些库：

```bash
cd PhysXWrapper
mkdir build && cd build
cmake .. -DPHYSX_ROOT=/home/user/PhysX/physx
make
```

PhysXWrapper 的 examples 会自动找到编译好的 PhysX 库。

## 故障排除

### CMake 找不到编译器

```bash
# 确保编译器已安装
which gcc g++
which clang clang++

# 安装 GCC
sudo apt-get install gcc g++

# 安装 Clang
sudo apt-get install clang
```

### Python3 找不到

```bash
sudo apt-get install python3
```

### 链接错误

如果出现链接错误，确保所有 PhysX 库都在同一目录：
```bash
ls -l physx/bin/linux.gcc/release/
```

## 技术细节

### 与原始构建系统的区别

1. **原始方法** (使用 packman):
   - 下载预编译的编译器和工具链
   - 使用 NVIDIA 特定版本的 GCC/Clang
   - 下载特殊的依赖包

2. **新方法** (使用系统库):
   - ✅ 使用系统已安装的 GCC/Clang
   - ✅ 使用系统 CMake
   - ✅ 无需网络下载 (PhysX)
   - ✅ 更简单、更快速
   - ⚠️ BLAST 和 FLOW 仍需特殊依赖

### PhysX CMake 配置

新的 build.sh 直接调用 CMake，设置以下关键参数：

```cmake
-DCMAKE_C_COMPILER=/usr/bin/gcc
-DCMAKE_CXX_COMPILER=/usr/bin/g++
-DCMAKE_BUILD_TYPE=release
-DCMAKE_MODULE_PATH=<PhysX cmake modules>
-DTARGET_BUILD_PLATFORM=linux
-DPX_OUTPUT_LIB_DIR=<output directory>
-DPX_OUTPUT_BIN_DIR=<output directory>
-DPX_BUILDSNIPPETS=OFF
-DPX_BUILDPVDRUNTIME=ON
-DPX_GENERATE_STATIC_LIBRARIES=OFF
-DPX_GENERATE_GPU_PROJECTS=OFF
```

## 总结

- ✅ **PhysX**: 完全支持系统库编译，无需 packman
- ⚠️ **BLAST**: 需要 NVIDIA 特殊构建工具或手动创建 CMake 配置
- ⚠️ **FLOW**: 需要 Slang 着色器编译器

建议：
1. 如果只需要 PhysX，使用新的 build.sh 即可
2. 如果需要 BLAST/FLOW，目前仍建议使用原始的 packman 系统
3. 未来可以考虑为 BLAST 创建独立的 CMakeLists.txt

## 参考

- [PhysX GitHub](https://github.com/NVIDIA-Omniverse/PhysX)
- [Slang Shader Compiler](https://github.com/shader-slang/slang)
- [CMake Documentation](https://cmake.org/documentation/)
