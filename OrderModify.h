#pragma once

#include "Order.h"  // 包含 Order 类的定义和所需的依赖（如 OrderId、Price、Side、Quantity 等）

// 定义订单修改类，用于创建或修改订单
class OrderModify
{
public:
    // 构造函数，接受订单 ID、买卖方向、价格和数量作为参数，并初始化相应的成员变量
    OrderModify(OrderId orderId, Side side, Price price, Quantity quantity)
            : orderId_{ orderId }      // 初始化订单 ID
            , price_{ price }          // 初始化订单价格
            , side_{ side }            // 初始化订单方向（买入或卖出）
            , quantity_{ quantity }    // 初始化订单数量
    { }

    // 获取订单 ID
    OrderId GetOrderId() const { return orderId_; }

    // 获取订单价格
    Price GetPrice() const { return price_; }

    // 获取订单方向（买入或卖出）
    Side GetSide() const { return side_; }

    // 获取订单数量
    Quantity GetQuantity() const { return quantity_; }

    // 将当前 OrderModify 对象转换为 Order 指针，接受一个订单类型参数
    OrderPointer ToOrderPointer(OrderType type) const
    {
        // 创建并返回一个共享指针指向新的 Order 对象，使用当前对象的属性和传入的订单类型
        return std::make_shared<Order>(type, GetOrderId(), GetSide(), GetPrice(), GetQuantity());
    }

private:
    OrderId orderId_;    // 保存订单 ID
    Price price_;        // 保存订单价格
    Side side_;          // 保存订单方向（买方或卖方）
    Quantity quantity_;  // 保存订单数量
};
