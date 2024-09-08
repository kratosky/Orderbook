#pragma once

#include "LevelInfo.h"

// 定义 OrderbookLevelInfos 类，用于存储订单簿的买单和卖单的级别信息
class OrderbookLevelInfos
{
public:
    // 构造函数，接受买单和卖单的级别信息
    OrderbookLevelInfos(const LevelInfos& bids, const LevelInfos& asks)
            : bids_{ bids }  // 初始化 bids_ 成员变量，使用初始化列表
            , asks_{ asks }  // 初始化 asks_ 成员变量，使用初始化列表
    { }

    // 获取买单级别信息的常量引用
    const LevelInfos& GetBids() const { return bids_; }

    // 获取卖单级别信息的常量引用
    const LevelInfos& GetAsks() const { return asks_; }

private:
    // 存储买单的级别信息，类型为 LevelInfos（从 LevelInfo.h 中定义）
    LevelInfos bids_;

    // 存储卖单的级别信息，类型为 LevelInfos（从 LevelInfo.h 中定义）
    LevelInfos asks_;
};


