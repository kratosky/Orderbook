#pragma once

enum class OrderType
{
	GoodTillCancel,
	FillAndKill,
	FillOrKill,
	GoodForDay,
	Market,
};
//GoodTillCancel (GTC)：订单会一直存在，直到被执行或取消。
//FillAndKill (FAK)：立即部分执行，未成交部分会被取消。
//FillOrKill (FOK)：必须全部立即成交，否则全部取消。
//GoodForDay (GFD)：当日有效，未完成部分在当天结束时取消。
//Market：按当前市场价格立即执行。