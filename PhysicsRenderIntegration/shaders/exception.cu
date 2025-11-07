// exception.cu - 异常处理程序

#include <optix.h>

/**
 * @brief OptiX 异常处理程序
 *
 * 当着色器出现错误时调用
 */
extern "C" __global__ void __exception__all() {
    // 获取异常代码
    const int exceptionCode = optixGetExceptionCode();

    // 获取像素坐标
    const uint3 launchIdx = optixGetLaunchIndex();

    // 设置错误颜色（品红色，方便识别）
    // 注意：这里需要访问输出缓冲区，但我们没有系统数据
    // 在实际实现中，应该通过 payload 或其他方式传递

    // 暂时：不做任何操作，避免崩溃
    // printf("OptiX Exception at (%u, %u): code %d\n",
    //        launchIdx.x, launchIdx.y, exceptionCode);
}
