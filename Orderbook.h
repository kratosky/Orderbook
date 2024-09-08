#pragma once

#include <map>
#include <unordered_map>
#include <thread>
#include <condition_variable>
#include <mutex>

#include "Usings.h"                     // 包含类型定义，如 OrderId、Price、Quantity 等
#include "Order.h"                      // 包含 Order 类的定义
#include "OrderModify.h"                // 包含 OrderModify 类的定义
#include "OrderbookLevelInfos.h"        // 包含 OrderbookLevelInfos 的定义，用于获取订单簿级别的信息
#include "Trade.h"                      // 包含 Trade 类的定义，用于存储交易信息

// 订单簿类定义
class Orderbook
{
private:

    // 用于存储订单条目的结构体，包含订单指针和订单在订单列表中的迭代器位置
    struct OrderEntry
    {
        OrderPointer order_{ nullptr };            // 订单指针
        OrderPointers::iterator location_;         // 订单在列表中的位置
    };

    // 用于存储订单簿级别数据的结构体，记录每个价格级别的数量和订单数
    struct LevelData
    {
        Quantity quantity_{ };                     // 当前价格级别的总订单数量
        Quantity count_{ };                        // 当前价格级别的订单数

        // 用于更新级别数据的操作类型
        enum class Action
        {
            Add,                                   // 新增订单
            Remove,                                // 移除订单
            Match,                                 // 部分或全部匹配订单
        };
    };

    // 保存订单簿的级别数据，价格为键，LevelData 为值
    std::unordered_map<Price, LevelData> data_;
    // 保存买单列表，按照价格从高到低排序
    std::map<Price, OrderPointers, std::greater<Price>> bids_;
    // 保存卖单列表，按照价格从低到高排序
    std::map<Price, OrderPointers, std::less<Price>> asks_;
    // 保存订单 ID 到订单条目映射，用于快速查找订单
    std::unordered_map<OrderId, OrderEntry> orders_;
    // 用于线程同步的互斥锁
    mutable std::mutex ordersMutex_;
    // 用于清理当日有效订单的后台线程
    std::thread ordersPruneThread_;
    // 条件变量，用于控制线程的关闭
    std::condition_variable shutdownConditionVariable_;
    // 标识是否关闭订单簿的标志
    std::atomic<bool> shutdown_{ false };

    // 清理当日有效订单的函数
    void PruneGoodForDayOrders();

    // 批量取消订单
    void CancelOrders(OrderIds orderIds);
    // 内部取消订单的实现
    void CancelOrderInternal(OrderId orderId);

    // 当订单被取消时的回调函数
    void OnOrderCancelled(OrderPointer order);
    // 当订单被添加时的回调函数
    void OnOrderAdded(OrderPointer order);
    // 当订单匹配时的回调函数
    void OnOrderMatched(Price price, Quantity quantity, bool isFullyFilled);
    // 更新价格级别数据
    void UpdateLevelData(Price price, Quantity quantity, LevelData::Action action);

    // 判断是否可以完全匹配某个订单
    bool CanFullyFill(Side side, Price price, Quantity quantity) const;
    // 判断是否可以匹配某个订单
    bool CanMatch(Side side, Price price) const;
    // 匹配订单并返回交易记录
    Trades MatchOrders();

public:

    // 构造函数
    Orderbook();
    // 禁用拷贝构造函数
    Orderbook(const Orderbook&) = delete;
    // 禁用拷贝赋值运算符
    void operator=(const Orderbook&) = delete;
    // 禁用移动构造函数
    Orderbook(Orderbook&&) = delete;
    // 禁用移动赋值运算符
    void operator=(Orderbook&&) = delete;
    // 析构函数
    ~Orderbook();

    // 添加订单并返回匹配的交易
    Trades AddOrder(OrderPointer order);
    // 取消订单
    void CancelOrder(OrderId orderId);
    // 修改订单并返回匹配的交易
    Trades ModifyOrder(OrderModify order);

    // 返回订单簿的大小（订单数量）
    std::size_t Size() const;
    // 获取当前订单簿的级别信息
    OrderbookLevelInfos GetOrderInfos() const;
};
