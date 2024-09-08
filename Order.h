#pragma once

#include <list>
#include <exception>
#include <format>

#include "OrderType.h"   // 包含订单类型的定义（如市场订单、限价订单等）
#include "Side.h"        // 包含订单方向的定义（买方或卖方）
#include "Usings.h"      // 包含一些类型定义，如 OrderId、Price、Quantity 等
#include "Constants.h"   // 包含常量定义，如无效价格（InvalidPrice）

// 定义订单类
class Order
{
public:
    // 构造函数，初始化订单对象，接受订单类型、订单 ID、买卖方向、价格、数量作为参数
    Order(OrderType orderType, OrderId orderId, Side side, Price price, Quantity quantity)
            : orderType_{ orderType }             // 初始化订单类型
            , orderId_{ orderId }                 // 初始化订单 ID
            , side_{ side }                       // 初始化订单方向
            , price_{ price }                     // 初始化订单价格
            , initialQuantity_{ quantity }        // 初始化初始订单数量
            , remainingQuantity_{ quantity }      // 初始化剩余订单数量为初始数量（订单未成交时相等）
    { }

    // 重载的构造函数，专门用于市场订单，价格设置为无效价格（市场订单不需要价格）
    Order(OrderId orderId, Side side, Quantity quantity)
            : Order(OrderType::Market, orderId, side, Constants::InvalidPrice, quantity)
    { }

    // 获取订单 ID
    OrderId GetOrderId() const { return orderId_; }

    // 获取订单的买卖方向
    Side GetSide() const { return side_; }

    // 获取订单价格
    Price GetPrice() const { return price_; }

    // 获取订单类型（如市场订单、限价订单）
    OrderType GetOrderType() const { return orderType_; }

    // 获取订单的初始数量
    Quantity GetInitialQuantity() const { return initialQuantity_; }

    // 获取订单的剩余数量
    Quantity GetRemainingQuantity() const { return remainingQuantity_; }

    // 获取订单已成交的数量（初始数量 - 剩余数量）
    Quantity GetFilledQuantity() const { return GetInitialQuantity() - GetRemainingQuantity(); }

    // 判断订单是否已全部成交（剩余数量为 0）
    bool IsFilled() const { return GetRemainingQuantity() == 0; }

    // 成交一定数量的订单
    void Fill(Quantity quantity)
    {
        // 如果成交数量超过剩余数量，则抛出逻辑错误异常
        if (quantity > GetRemainingQuantity())
            throw std::logic_error(std::format("Order ({}) cannot be filled for more than its remaining quantity.", GetOrderId()));

        // 更新剩余数量，减少成交的数量
        remainingQuantity_ -= quantity;
    }

    // 将订单类型转换为“Good Till Cancel”（GTC：有效期至取消）并设置新的价格
    void ToGoodTillCancel(Price price)
    {
        // 如果订单不是市场订单，则抛出逻辑错误异常，因为只有市场订单允许价格调整
        if (GetOrderType() != OrderType::Market)
            throw std::logic_error(std::format("Order ({}) cannot have its price adjusted, only market orders can.", GetOrderId()));

        // 设置新的价格并将订单类型变更为 GTC
        price_ = price;
        orderType_ = OrderType::GoodTillCancel;
    }

private:
    OrderType orderType_;          // 订单类型（市场订单、限价订单等）
    OrderId orderId_;              // 订单 ID（唯一标识符）
    Side side_;                    // 订单方向（买入或卖出）
    Price price_;                  // 订单价格
    Quantity initialQuantity_;     // 订单的初始数量
    Quantity remainingQuantity_;   // 订单的剩余数量
};

// 使用智能指针管理 Order 对象
using OrderPointer = std::shared_ptr<Order>;

// 订单指针列表类型
using OrderPointers = std::list<OrderPointer>;
