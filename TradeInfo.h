#pragma once

#include "Usings.h"

// 定义一个结构体 TradeInfo，用于存储有关交易的信息
struct TradeInfo
{
    OrderId orderId_;   // 订单的唯一标识符，类型为 OrderId（从 Usings.h 中定义）
    Price price_;       // 订单的交易价格，类型为 Price（从 Usings.h 中定义）
    Quantity quantity_; // 订单的交易数量，类型为 Quantity（从 Usings.h 中定义）
};


