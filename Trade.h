#pragma once

#include "TradeInfo.h"

// 定义 Trade 类，表示一次买卖双方的成交信息
class Trade
{
public:
    // 构造函数，接受买单和卖单的成交信息
    Trade(const TradeInfo& bidTrade, const TradeInfo& askTrade)
            : bidTrade_{ bidTrade }  // 使用初始化列表初始化 bidTrade_ 成员
            , askTrade_{ askTrade }  // 使用初始化列表初始化 askTrade_ 成员
    { }

    // 获取买单的成交信息
    const TradeInfo& GetBidTrade() const { return bidTrade_; }

    // 获取卖单的成交信息
    const TradeInfo& GetAskTrade() const { return askTrade_; }

private:
    // 买单的成交信息
    TradeInfo bidTrade_;

    // 卖单的成交信息
    TradeInfo askTrade_;
};

// 定义 Trades 类型，用于存储多个 Trade 对象
using Trades = std::vector<Trade>;


