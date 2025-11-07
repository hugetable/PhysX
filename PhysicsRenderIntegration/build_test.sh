#!/bin/bash
# build_test.sh - 测试构建脚本

echo "=================================="
echo "PhysicsRenderIntegration 构建测试"
echo "=================================="
echo ""

# 设置环境变量
export OPTIX90_PATH=/home/user/OptiX-sdk
export PHYSX_ROOT_DIR=/home/user/PhysX
export CUDACXX=/usr/local/cuda/bin/nvcc

echo "环境配置:"
echo "  OPTIX90_PATH=$OPTIX90_PATH"
echo "  PHYSX_ROOT_DIR=$PHYSX_ROOT_DIR"
echo "  CUDACXX=$CUDACXX"
echo ""

# 创建构建目录
cd /home/user/PhysX/PhysicsRenderIntegration

if [ -d "build" ]; then
    echo "清理旧的构建目录..."
    rm -rf build
fi

mkdir build
cd build

echo "配置 CMake..."
cmake .. \
    -DOPTIX90_PATH=$OPTIX90_PATH \
    -DPHYSX_ROOT_DIR=$PHYSX_ROOT_DIR \
    2>&1 | tee cmake_config.log

if [ ${PIPESTATUS[0]} -ne 0 ]; then
    echo ""
    echo "❌ CMake 配置失败！"
    echo "请查看 cmake_config.log"
    exit 1
fi

echo ""
echo "CMake 配置成功！"
echo ""
echo "开始编译..."
make -j$(nproc) 2>&1 | tee make.log

if [ ${PIPESTATUS[0]} -ne 0 ]; then
    echo ""
    echo "❌ 编译失败！"
    echo "请查看 make.log"
    exit 1
fi

echo ""
echo "✅ 编译成功！"
echo ""
echo "生成的文件："
ls -lh bin/

echo ""
echo "=================================="
echo "构建测试完成"
echo "=================================="
