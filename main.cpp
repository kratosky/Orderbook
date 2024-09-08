#include <iostream>
#include "Orderbook.h"

//int main()
//{
//    Orderbook orderbook;
//    // Do work.
////    const OrderId orderId = 1;
////    orderbook.AddOrder(std::make_shared<Order>(OrderType::GoodTillCancel,orderId,Side::Buy,100,10));
////    std::cout<<orderbook.Size()<<std::endl;//1
////    orderbook.CancelOrder(orderId);
////    std::cout<<orderbook.Size()<<std::endl;//0
//
//    return 0;
//}
#include <gtest/gtest.h>

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS(); // 这行代码会运行所有定义的测试
}