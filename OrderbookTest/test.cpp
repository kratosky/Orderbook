#include "pch.h"

#include "../Orderbook.h"  // 引入 Orderbook 类的定义

namespace googletest = ::testing;  // 为 Google Test 命名空间定义别名

// 定义一个枚举，用于表示不同的动作类型
enum class ActionType
{
    Add,    // 添加订单
    Cancel, // 取消订单
    Modify, // 修改订单
};

// 定义结构体，用于存储订单的相关信息
struct Information
{
    ActionType type_;    // 动作类型（添加、取消或修改）
    OrderType orderType_; // 订单类型（例如市价订单、限价订单等）
    Side side_;          // 买方或卖方
    Price price_;        // 订单价格
    Quantity quantity_;  // 订单数量
    OrderId orderId_;    // 订单 ID
};

// 使用别名，将多个 Information 对象存储在一个 vector 中
using Informations = std::vector<Information>;

// 定义结构体，用于存储测试的期望结果
struct Result
{
    std::size_t allCount_; // 订单总数
    std::size_t bidCount_; // 买单总数
    std::size_t askCount_; // 卖单总数
};

// 使用别名，将多个 Result 对象存储在一个 vector 中
using Results = std::vector<Result>;

// 定义处理输入的结构体
struct InputHandler
{
private:
    // 将字符串转换为数字
    std::uint32_t ToNumber(const std::string_view& str) const
    {
        std::int64_t value{};
        // 使用 std::from_chars 将字符串转换为整数值
        std::from_chars(str.data(), str.data() + str.size(), value);
        if (value < 0)
            throw std::logic_error("Value is below zero."); // 如果值小于 0，抛出异常
        return static_cast<std::uint32_t>(value); // 将整数转换为无符号 32 位整数
    }

    // 尝试解析结果行，将其解析为 Result 结构体
    bool TryParseResult(const std::string_view& str, Result& result) const
    {
        if (str.at(0) != 'R') // 如果行的第一个字符不是 'R'，则返回 false
            return false;

        auto values = Split(str, ' '); // 使用空格分割字符串
        // 将分割的结果转换为 Result 结构体中的各个字段
        result.allCount_ = ToNumber(values[1]);
        result.bidCount_ = ToNumber(values[2]);
        result.askCount_ = ToNumber(values[3]);

        return true; // 返回解析成功
    }

    // 尝试解析一行输入，将其解析为 Information 结构体
    bool TryParseInformation(const std::string_view& str, Information& action) const
    {
        auto value = str.at(0); // 获取第一个字符
        auto values = Split(str, ' '); // 使用空格分割字符串
        if (value == 'A') // 如果是添加订单
        {
            action.type_ = ActionType::Add; // 设置类型为 Add
            action.side_ = ParseSide(values[1]); // 解析买卖方向
            action.orderType_ = ParseOrderType(values[2]); // 解析订单类型
            action.price_ = ParsePrice(values[3]); // 解析价格
            action.quantity_ = ParseQuantity(values[4]); // 解析数量
            action.orderId_ = ParseOrderId(values[5]); // 解析订单 ID
        }
        else if (value == 'M') // 如果是修改订单
        {
            action.type_ = ActionType::Modify; // 设置类型为 Modify
            action.orderId_ = ParseOrderId(values[1]); // 解析订单 ID
            action.side_ = ParseSide(values[2]); // 解析买卖方向
            action.price_ = ParsePrice(values[3]); // 解析价格
            action.quantity_ = ParseQuantity(values[4]); // 解析数量
        }
        else if (value == 'C') // 如果是取消订单
        {
            action.type_ = ActionType::Cancel; // 设置类型为 Cancel
            action.orderId_ = ParseOrderId(values[1]); // 解析订单 ID
        }
        else return false; // 如果是其他字符，返回 false

        return true; // 返回解析成功
    }

    // 将字符串按指定的分隔符分割为多个部分
    std::vector<std::string_view> Split(const std::string_view& str, char delimeter) const
    {
        std::vector<std::string_view> columns;
        columns.reserve(5); // 预分配 5 个元素的空间
        std::size_t start_index{}, end_index{};
        // 循环查找分隔符位置
        while ((end_index = str.find(delimeter, start_index)) && end_index != std::string::npos)
        {
            auto distance = end_index - start_index;
            auto column = str.substr(start_index, distance); // 提取子字符串
            start_index = end_index + 1;
            columns.push_back(column); // 将子字符串加入结果
        }
        columns.push_back(str.substr(start_index)); // 将最后一个子字符串加入结果
        return columns;
    }

    // 解析买卖方向（Buy 或 Sell）
    Side ParseSide(const std::string_view& str) const
    {
        if (str == "B")
            return Side::Buy; // 如果是 "B"，返回 Buy
        else if (str == "S")
            return Side::Sell; // 如果是 "S"，返回 Sell
        else throw std::logic_error("Unknown Side"); // 否则抛出异常
    }

    // 解析订单类型
    OrderType ParseOrderType(const std::string_view& str) const
    {
        if (str == "FillAndKill")
            return OrderType::FillAndKill;
        else if (str == "GoodTillCancel")
            return OrderType::GoodTillCancel;
        else if (str == "GoodForDay")
            return OrderType::GoodForDay;
        else if (str == "FillOrKill")
            return OrderType::FillOrKill;
        else if (str == "Market")
            return OrderType::Market;
        else throw std::logic_error("Unknown OrderType"); // 如果无法识别，抛出异常
    }

    // 解析价格
    Price ParsePrice(const std::string_view& str) const
    {
        if (str.empty())
            throw std::logic_error("Unknown Price"); // 如果字符串为空，抛出异常

        return ToNumber(str); // 将字符串转换为数字
    }

    // 解析数量
    Quantity ParseQuantity(const std::string_view& str) const
    {
        if (str.empty())
            throw std::logic_error("Unknown Quantity"); // 如果字符串为空，抛出异常

        return ToNumber(str); // 将字符串转换为数字
    }

    // 解析订单 ID
    OrderId ParseOrderId(const std::string_view& str) const
    {
        if (str.empty())
            throw std::logic_error("Empty OrderId"); // 如果字符串为空，抛出异常

        return static_cast<OrderId>(ToNumber(str)); // 将字符串转换为数字并强制转换为 OrderId 类型
    }

public:
    // 从文件路径中获取订单信息和预期结果
    std::tuple<Informations, Result> GetInformations(const std::filesystem::path& path) const
    {
        Informations actions;
        actions.reserve(1'000); // 预分配 1000 个元素的空间

        std::string line;
        std::ifstream file{ path }; // 打开文件
        while (std::getline(file, line)) // 逐行读取文件
        {
            if (line.empty())
                break; // 如果读取到空行，退出循环

            const bool isResult = line.at(0) == 'R'; // 判断是否为结果行
            const bool isAction = !isResult; // 判断是否为动作行

            if (isAction) // 如果是动作行
            {
                Information action;

                auto isValid = TryParseInformation(line, action); // 解析动作
                if (!isValid)
                    continue; // 如果解析失败，跳过该行

                actions.push_back(action); // 将动作加入 actions 列表
            }
            else // 如果是结果行
            {
                if (!file.eof()) // 如果文件未读到末尾
                    throw std::logic_error("Result should only be specified at the end."); // 抛出异常

                Result result;

                auto isValid = TryParseResult(line, result); // 解析结果行
                if (!isValid)
                    continue; // 如果解析失败，跳过该行

                return { actions, result }; // 返回解析结果
            }

        }

        throw std::logic_error("No result specified."); // 如果没有结果行，抛出异常
    }
};

// 测试套件的基类，继承自 Google Test 框架
class OrderbookTestsFixture : public googletest::TestWithParam<const char*>
{
private:
    // 定义文件路径常量
    const static inline std::filesystem::path Root{ std::filesystem::current_path().parent_path() };  // 获取项目根目录
    const static inline std::filesystem::path TestFolder{ "OrderbookTest/TestFiles" };
public:
    const static inline std::filesystem::path TestFolderPath{ Root / TestFolder }; // 测试文件夹的路径
};

// 定义测试用例
TEST_P(OrderbookTestsFixture, OrderbookTestSuite)
{
    // Arrange: 准备阶段
    const auto file = OrderbookTestsFixture::TestFolderPath / GetParam(); // 获取测试文件的完整路径
    std::cout << "Testing file: " << file << std::endl;  // 输出文件路径

    InputHandler handler;
    const auto [actions, result] = handler.GetInformations(file); // 从文件中获取动作和预期结果

    // Lambda 函数，用于创建订单
    auto GetOrder = [](const Information& action)
    {
        return std::make_shared<Order>(
                action.orderType_,
                action.orderId_,
                action.side_,
                action.price_,
                action.quantity_);
    };

    // Lambda 函数，用于创建订单修改操作
    auto GetOrderModify = [](const Information& action)
    {
        return OrderModify
                {
                        action.orderId_,
                        action.side_,
                        action.price_,
                        action.quantity_,
                };
    };

    // Act: 执行阶段
    Orderbook orderbook;
    for (const auto& action : actions)
    {
        switch (action.type_)
        {
            case ActionType::Add: // 添加订单
            {
                const Trades& trades = orderbook.AddOrder(GetOrder(action)); // 添加订单并返回成交结果
            }
                break;
            case ActionType::Modify: // 修改订单
            {
                const Trades& trades = orderbook.ModifyOrder(GetOrderModify(action)); // 修改订单并返回成交结果
            }
                break;
            case ActionType::Cancel: // 取消订单
            {
                orderbook.CancelOrder(action.orderId_); // 取消订单
            }
                break;
            default:
                throw std::logic_error("Unsupported Action."); // 如果遇到不支持的操作，抛出异常
        }
    }

    // Assert: 断言阶段，检查结果是否符合预期
    const auto& orderbookInfos = orderbook.GetOrderInfos();
    ASSERT_EQ(orderbook.Size(), result.allCount_); // 检查订单簿中的订单总数
    ASSERT_EQ(orderbookInfos.GetBids().size(), result.bidCount_); // 检查买单的数量
    ASSERT_EQ(orderbookInfos.GetAsks().size(), result.askCount_); // 检查卖单的数量
}

// 使用参数化测试，将多个测试文件传递给测试用例
INSTANTIATE_TEST_SUITE_P(Tests, OrderbookTestsFixture, googletest::ValuesIn({
        "Match_GoodTillCancel.txt",
        "Match_FillAndKill.txt",
        "Match_FillOrKill_Hit.txt",
        "Match_FillOrKill_Miss.txt",
        "Cancel_Success.txt",
        "Modify_Side.txt",
        "Match_Market.txt"
}));
