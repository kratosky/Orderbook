#pragma once

#include <limits>  // 引入标准库 <limits>，用于处理类型的极值（如最大、最小值，以及 NaN）

#include "Usings.h"  // 引入 "Usings.h"，其中定义了 Price 类型的别名

// 定义 Constants 结构体，用于存储全局常量
struct Constants
{
    // 静态常量 InvalidPrice，表示无效的价格，值为 Price 类型的 NaN（Not-a-Number）
    // std::numeric_limits<Price>::quiet_NaN() 返回 NaN 值（适用于浮点数类型）
    static const Price InvalidPrice = std::numeric_limits<Price>::quiet_NaN();
};

