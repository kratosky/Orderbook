#pragma once

#include "Usings.h"

// 定义一个结构体 LevelInfo，用于存储单个价格级别的相关信息
struct LevelInfo
{
    Price price_;       // 存储价格级别的价格值，类型为 Price（从 Usings.h 中定义）
    Quantity quantity_; // 存储该价格级别的订单总量，类型为 Quantity（从 Usings.h 中定义）
};

// 使用别名定义 LevelInfos，为存储多个 LevelInfo 对象的 std::vector
using LevelInfos = std::vector<LevelInfo>;

