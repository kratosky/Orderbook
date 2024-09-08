#pragma once

#include <vector>  // 引入标准库中的 vector 容器，用于存储多个 OrderId

// 使用 using 关键字为基本数据类型定义别名，使代码更具可读性和语义化

// 定义 Price 为 32 位整数类型，表示价格。使用 int32_t 可以确保跨平台的一致性。
using Price = std::int32_t;

// 定义 Quantity 为无符号 32 位整数类型，表示订单的数量。无符号类型保证数量不能为负数。
using Quantity = std::uint32_t;

// 定义 OrderId 为无符号 64 位整数类型，表示订单的唯一标识符。较大的范围适合标识长时间运行系统中的大量订单。
using OrderId = std::uint64_t;

// 定义 OrderIds 为一个存储多个 OrderId 的向量类型，使用 vector 容器来动态存储多个订单标识符。
using OrderIds = std::vector<OrderId>;

