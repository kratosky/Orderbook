cmake_minimum_required(VERSION 3.27)
project(Orderbook)

set(CMAKE_CXX_STANDARD 20)

include_directories(.)
include_directories(OrderbookTest)

# 下载和配置 GoogleTest
include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/release-1.11.0.zip
)
# 可选项：为了防止不必要的多次下载，启用缓存
FetchContent_MakeAvailable(googletest)

enable_testing()

# 添加你项目的源文件和测试文件
add_executable(Orderbook
#        OrderbookTest/pch.cpp
        OrderbookTest/pch.h
        OrderbookTest/test.cpp
        Constants.h
        LevelInfo.h
        main.cpp
        Order.h
        Orderbook.cpp
        Orderbook.h
        OrderbookLevelInfos.h
        OrderModify.h
        OrderType.h
        Side.h
        Trade.h
        TradeInfo.h
        Usings.h)

# 链接 GoogleTest 库
target_link_libraries(Orderbook gtest gtest_main)

# 添加测试目标
add_test(NAME OrderbookTest COMMAND Orderbook)
