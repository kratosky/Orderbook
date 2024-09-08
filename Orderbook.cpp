
#include "Orderbook.h"
#include <numeric>
#include <chrono>
#include <ctime>

// 清理当日有效订单的函数
void Orderbook::PruneGoodForDayOrders()
{
    using namespace std::chrono;
    const auto end = hours(16);  // 设定交易日结束时间为下午 4 点

    while (true)
    {
        // 获取当前系统时间
        const auto now = system_clock::now();
        const auto now_c = system_clock::to_time_t(now);  // 转换为 time_t 类型
        std::tm now_parts;                                // 创建时间结构体
        localtime_s(&now_parts, &now_c);                  // 将 time_t 转换为本地时间格式

        // 如果时间已经超过 4 点，将时间调整到第二天
        if (now_parts.tm_hour >= end.count())
            now_parts.tm_mday += 1;

        // 设置清理时间为第二天的 4 点
        now_parts.tm_hour = end.count();
        now_parts.tm_min = 0;
        now_parts.tm_sec = 0;

        // 计算下一个 4 点的时间
        auto next = system_clock::from_time_t(mktime(&now_parts));
        // 计算到达下一个 4 点的剩余时间
        auto till = next - now + milliseconds(100);

        {
            // 通过互斥锁锁定订单列表
            std::unique_lock ordersLock{ ordersMutex_ };

            // 如果线程需要关闭或者条件变量被唤醒，则退出
            if (shutdown_.load(std::memory_order_acquire) ||
                shutdownConditionVariable_.wait_for(ordersLock, till) == std::cv_status::no_timeout)
                return;
        }

        OrderIds orderIds;

        {
            // 使用 scoped_lock 锁定订单映射
            std::scoped_lock ordersLock{ ordersMutex_ };

            // 遍历所有订单，收集 GoodForDay 类型的订单 ID
            for (const auto& [_, entry] : orders_)
            {
                const auto& [order, placeholder] = entry;

                if (order->GetOrderType() != OrderType::GoodForDay)
                    continue;

                // 如果订单是 GoodForDay 类型，则将其 ID 放入 orderIds 列表中
                orderIds.push_back(order->GetOrderId());
            }
        }

        // 调用取消订单的函数
        CancelOrders(orderIds);
    }
}

// 批量取消订单
void Orderbook::CancelOrders(OrderIds orderIds)
{
    // 使用 scoped_lock 锁定订单列表
    std::scoped_lock ordersLock{ ordersMutex_ };

    // 遍历订单 ID 列表，依次取消每个订单
    for (const auto& orderId : orderIds)
        CancelOrderInternal(orderId);
}

// 内部函数：处理订单取消的具体逻辑
void Orderbook::CancelOrderInternal(OrderId orderId)
{
    // 如果订单不存在，则直接返回
    if (!orders_.contains(orderId))
        return;

    // 获取订单指针和订单位置的迭代器
    const auto [order, iterator] = orders_.at(orderId);
    orders_.erase(orderId);  // 从订单映射中删除订单

    // 根据订单方向，处理买单或卖单列表
    if (order->GetSide() == Side::Sell)
    {
        // 获取价格和对应的卖单列表
        auto price = order->GetPrice();
        auto& orders = asks_.at(price);
        // 从卖单列表中删除该订单
        orders.erase(iterator);
        // 如果该价格级别的订单为空，删除该价格级别
        if (orders.empty())
            asks_.erase(price);
    }
    else
    {
        // 获取价格和对应的买单列表
        auto price = order->GetPrice();
        auto& orders = bids_.at(price);
        // 从买单列表中删除该订单
        orders.erase(iterator);
        // 如果该价格级别的订单为空，删除该价格级别
        if (orders.empty())
            bids_.erase(price);
    }

    // 调用订单取消的回调函数
    OnOrderCancelled(order);
}

// 当订单被取消时，更新订单簿数据
void Orderbook::OnOrderCancelled(OrderPointer order)
{
    UpdateLevelData(order->GetPrice(), order->GetRemainingQuantity(), LevelData::Action::Remove);
}

// 当新订单被添加时，更新订单簿数据
void Orderbook::OnOrderAdded(OrderPointer order)
{
    UpdateLevelData(order->GetPrice(), order->GetInitialQuantity(), LevelData::Action::Add);
}

// 当订单被匹配时，更新订单簿数据
void Orderbook::OnOrderMatched(Price price, Quantity quantity, bool isFullyFilled)
{
    // 如果订单完全成交，则删除该订单的数据；否则只更新数量
    UpdateLevelData(price, quantity, isFullyFilled ? LevelData::Action::Remove : LevelData::Action::Match);
}

// 更新订单簿价格级别的数据
void Orderbook::UpdateLevelData(Price price, Quantity quantity, LevelData::Action action)
{
    // 获取或创建该价格级别的级别数据
    auto& data = data_[price];

    // 根据操作类型更新该价格级别的订单数量和订单数
    data.count_ += action == LevelData::Action::Remove ? -1 : action == LevelData::Action::Add ? 1 : 0;
    if (action == LevelData::Action::Remove || action == LevelData::Action::Match)
    {
        data.quantity_ -= quantity;  // 如果订单被移除或匹配，减少数量
    }
    else
    {
        data.quantity_ += quantity;  // 如果新增订单，增加数量
    }

    // 如果该价格级别的订单数为 0，则删除该价格级别
    if (data.count_ == 0)
        data_.erase(price);
}

// 判断是否可以完全匹配某个订单
bool Orderbook::CanFullyFill(Side side, Price price, Quantity quantity) const
{
    // 如果不能匹配该订单，则直接返回 false
    if (!CanMatch(side, price))
        return false;

    std::optional<Price> threshold;

    // 如果是买单，获取最佳卖价作为匹配的阈值
    if (side == Side::Buy)
    {
        const auto [askPrice, _] = *asks_.begin();
        threshold = askPrice;
    }
    else
    {
        // 如果是卖单，获取最佳买价作为匹配的阈值
        const auto [bidPrice, _] = *bids_.begin();
        threshold = bidPrice;
    }

    // 遍历订单簿，检查是否可以完全成交
    for (const auto& [levelPrice, levelData] : data_)
    {
        // 如果当前级别的价格不满足匹配条件，则跳过
        //这里比对手方最佳还好的价格是与自己同一边的价格故舍弃
        if (threshold.has_value() &&
            (side == Side::Buy && threshold.value() > levelPrice) ||
            (side == Side::Sell && threshold.value() < levelPrice))
            continue;
        //这里是不满足下单条件的价格
        if ((side == Side::Buy && levelPrice > price) ||
            (side == Side::Sell && levelPrice < price))
            continue;

        // 如果当前级别的订单数量足以满足需求，则返回 true
        if (quantity <= levelData.quantity_)
            return true;

        // 否则减少数量，继续检查下一个级别
        quantity -= levelData.quantity_;
    }

    // 如果所有级别的数量不足以完全成交，返回 false
    return false;
}

// 判断是否可以匹配某个订单
bool Orderbook::CanMatch(Side side, Price price) const
{
    // 如果是买单，检查是否存在卖单，并且卖单价格符合匹配条件
    if (side == Side::Buy)
    {
        if (asks_.empty())
            return false;

        const auto& [bestAsk, _] = *asks_.begin();
        return price >= bestAsk;
    }
    else
    {
        // 如果是卖单，检查是否存在买单，并且买单价格符合匹配条件
        if (bids_.empty())
            return false;

        const auto& [bestBid, _] = *bids_.begin();
        return price <= bestBid;
    }
}

// 匹配买单和卖单，并生成交易记录
Trades Orderbook::MatchOrders()
{
    Trades trades;  // 存储匹配结果的交易记录
    trades.reserve(orders_.size());  // 预留空间

    while (true)
    {
        // 如果买单或卖单列表为空，则退出匹配过程
        if (bids_.empty() || asks_.empty())
            break;

        // 获取最佳买单和卖单的价格及订单列表
        auto& [bidPrice, bids] = *bids_.begin();
        auto& [askPrice, asks] = *asks_.begin();

        // 如果最佳买价低于最佳卖价，无法匹配，退出
        if (bidPrice < askPrice)
            break;

        // 遍历买单和卖单，进行匹配
        while (!bids.empty() && !asks.empty())
        {
            auto bid = bids.front();  // 获取当前的买单
            auto ask = asks.front();  // 获取当前的卖单

            // 计算可以成交的数量
            Quantity quantity = std::min(bid->GetRemainingQuantity(), ask->GetRemainingQuantity());

            // 更新买单和卖单的成交数量
            bid->Fill(quantity);
            ask->Fill(quantity);

            // 如果买单已完全成交，从列表中删除该买单
            if (bid->IsFilled())
            {
                bids.pop_front();
                orders_.erase(bid->GetOrderId());
            }

            // 如果卖单已完全成交，从列表中删除该卖单
            if (ask->IsFilled())
            {
                asks.pop_front();
                orders_.erase(ask->GetOrderId());
            }

            // 将此次交易信息记录到交易列表中
            trades.push_back(Trade{
                    TradeInfo{ bid->GetOrderId(), bid->GetPrice(), quantity },
                    TradeInfo{ ask->GetOrderId(), ask->GetPrice(), quantity }
            });

            // 更新订单簿数据
            OnOrderMatched(bid->GetPrice(), quantity, bid->IsFilled());
            OnOrderMatched(ask->GetPrice(), quantity, ask->IsFilled());
        }

        // 如果所有买单已匹配完，删除买单价格级别
        if (bids.empty())
        {
            bids_.erase(bidPrice);
            data_.erase(bidPrice);
        }

        // 如果所有卖单已匹配完，删除卖单价格级别
        if (asks.empty())
        {
            asks_.erase(askPrice);
            data_.erase(askPrice);
        }
    }

    // 处理 FillAndKill 类型的订单，如果无法匹配则取消订单
    if (!bids_.empty())
    {
        auto& [_, bids] = *bids_.begin();
        auto& order = bids.front();
        if (order->GetOrderType() == OrderType::FillAndKill)
            CancelOrder(order->GetOrderId());
    }

    if (!asks_.empty())
    {
        auto& [_, asks] = *asks_.begin();
        auto& order = asks.front();
        if (order->GetOrderType() == OrderType::FillAndKill)
            CancelOrder(order->GetOrderId());
    }

    return trades;
}

// 构造函数，启动清理当日有效订单的线程
Orderbook::Orderbook() : ordersPruneThread_{ [this] { PruneGoodForDayOrders(); } } { }

// 析构函数，关闭订单簿并等待清理线程退出
Orderbook::~Orderbook()
{
    shutdown_.store(true, std::memory_order_release);   // 设置关闭标志
    shutdownConditionVariable_.notify_one();           // 唤醒清理线程
    ordersPruneThread_.join();                         // 等待线程结束
}

// 添加订单并匹配，返回交易记录
Trades Orderbook::AddOrder(OrderPointer order)
{
    std::scoped_lock ordersLock{ ordersMutex_ };  // 锁定订单列表

    // 如果订单已存在，返回空的交易记录
    if (orders_.contains(order->GetOrderId()))
        return { };

    // 如果是市场订单，自动调整为 GoodTillCancel 类型
    if (order->GetOrderType() == OrderType::Market)
    {
        // 对买单和卖单分别处理，根据最差的卖价或买价设置价格
        if (order->GetSide() == Side::Buy && !asks_.empty())
        {
            const auto& [worstAsk, _] = *asks_.rbegin();
            order->ToGoodTillCancel(worstAsk);
        }
        else if (order->GetSide() == Side::Sell && !bids_.empty())
        {
            const auto& [worstBid, _] = *bids_.rbegin();
            order->ToGoodTillCancel(worstBid);
        }
        else
            return { };  // 如果没有匹配的价格，直接返回
    }

    // 如果订单是 FillAndKill 类型，但无法匹配，则返回空的交易记录
    if (order->GetOrderType() == OrderType::FillAndKill && !CanMatch(order->GetSide(), order->GetPrice()))
        return { };

    // 如果订单是 FillOrKill 类型，但无法完全匹配，则返回空的交易记录
    if (order->GetOrderType() == OrderType::FillOrKill && !CanFullyFill(order->GetSide(), order->GetPrice(), order->GetInitialQuantity()))
        return { };

    OrderPointers::iterator iterator;

    // 根据订单方向，将订单插入到买单或卖单列表中
    if (order->GetSide() == Side::Buy)
    {
        auto& orders = bids_[order->GetPrice()];
        orders.push_back(order);
        iterator = std::prev(orders.end());
    }
    else
    {
        auto& orders = asks_[order->GetPrice()];
        orders.push_back(order);
        iterator = std::prev(orders.end());
    }

    // 在订单映射中记录订单信息
    orders_.insert({ order->GetOrderId(), OrderEntry{ order, iterator } });

    // 调用订单添加的回调函数
    OnOrderAdded(order);

    // 尝试匹配订单，并返回匹配结果
    return MatchOrders();
}

// 取消订单
void Orderbook::CancelOrder(OrderId orderId)
{
    std::scoped_lock ordersLock{ ordersMutex_ };  // 锁定订单列表

    CancelOrderInternal(orderId);  // 调用内部函数取消订单
}

// 修改订单，先取消原订单，再添加修改后的订单
Trades Orderbook::ModifyOrder(OrderModify order)
{
    OrderType orderType;

    {
        std::scoped_lock ordersLock{ ordersMutex_ };  // 锁定订单列表

        // 如果订单不存在，返回空的交易记录
        if (!orders_.contains(order.GetOrderId()))
            return { };

        // 获取现有订单的类型
        const auto& [existingOrder, _] = orders_.at(order.GetOrderId());
        orderType = existingOrder->GetOrderType();
    }

    // 取消原订单，并添加修改后的订单
    CancelOrder(order.GetOrderId());
    return AddOrder(order.ToOrderPointer(orderType));
}

// 返回订单簿中的订单数量
std::size_t Orderbook::Size() const
{
    std::scoped_lock ordersLock{ ordersMutex_ };  // 锁定订单列表
    return orders_.size();  // 返回订单数量
}

// 获取订单簿中的级别信息
OrderbookLevelInfos Orderbook::GetOrderInfos() const
{
    // 创建两个容器用于存储买单和卖单的级别信息
    LevelInfos bidInfos, askInfos;

    // 预留空间，以减少向量动态扩容的开销
    // reserve 函数根据当前的订单数量（orders_.size()）为 bidInfos 和 askInfos 预留足够的内存空间
    bidInfos.reserve(orders_.size());  // 为买单列表预留空间
    askInfos.reserve(orders_.size());  // 为卖单列表预留空间

    // Lambda 函数，用于创建 LevelInfo（价格和该价格级别的订单总数量）
    // 该函数接收价格（Price）和订单列表（OrderPointers），计算该价格级别的总订单数量
    auto CreateLevelInfos = [](Price price, const OrderPointers& orders)
    {
        // 返回一个 LevelInfo 对象
        // 使用 std::accumulate 来计算该价格级别下的订单总数量
        // std::accumulate 遍历 orders 列表，并累加每个订单的剩余数量（GetRemainingQuantity）
        return LevelInfo{ price, std::accumulate(orders.begin(), orders.end(), (Quantity)0,
                // Lambda 函数，累加每个订单的剩余数量
                                                 [](Quantity runningSum, const OrderPointer& order)
                                                 {
                                                     // 将当前订单的剩余数量加到累加器中
                                                     return runningSum + order->GetRemainingQuantity();
                                                 }) };
    };

    // 遍历买单价格级别，使用 CreateLevelInfos 生成每个价格级别的 LevelInfo，并添加到 bidInfos 向量中
    for (const auto& [price, orders] : bids_)
        bidInfos.push_back(CreateLevelInfos(price, orders));

    // 遍历卖单价格级别，使用 CreateLevelInfos 生成每个价格级别的 LevelInfo，并添加到 askInfos 向量中
    for (const auto& [price, orders] : asks_)
        askInfos.push_back(CreateLevelInfos(price, orders));

    // 返回包含买单和卖单级别信息的 OrderbookLevelInfos 对象
    return OrderbookLevelInfos{ bidInfos, askInfos };
}

