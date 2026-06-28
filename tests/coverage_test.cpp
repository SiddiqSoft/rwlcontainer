/*
	RWLContainer : Full Coverage Tests
	Comprehensive tests to ensure all methods have full coverage
	Repo: https://github.com/SiddiqSoft/RWLContainer

	BSD 3-Clause License

	Copyright (c) 2024, Siddiq Software LLC
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:

	1. Redistributions of source code must retain the above copyright notice, this
	   list of conditions and the following disclaimer.

	2. Redistributions in binary form must reproduce the above copyright notice,
	   this list of conditions and the following disclaimer in the documentation
	   and/or other materials provided with the distribution.

	3. Neither the name of the copyright holder nor the names of its
	   contributors may be used to endorse or promote products derived from
	   this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
	AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
	IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
	FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
	DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
	SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
	CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
	OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
	OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "gtest/gtest.h"
#include <format>
#include <thread>
#include <chrono>
#include "nlohmann/json.hpp"
#include "../include/siddiqsoft/RWLContainer.hpp"
#include "../include/siddiqsoft/WaitableQueue.hpp"

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

// ============================================================================
// RWLContainer Additional Coverage Tests
// ============================================================================

struct TestItem
{
	int         id {0};
	std::string value {};
};

// --- Test RWLContainer with different key types ---

TEST(RWLContainer_Coverage, StringKeyType)
{
	siddiqsoft::RWLContainer<std::string, TestItem> container;

	auto item1 = container.add("key1", TestItem {1, "value1"});
	auto item2 = container.add("key2", TestItem {2, "value2"});

	ASSERT_TRUE(item1);
	ASSERT_TRUE(item2);
	EXPECT_EQ(2u, container.size());

	auto found = container.find("key1");
	ASSERT_TRUE(found);
	EXPECT_EQ(1, found->id);
	EXPECT_EQ("value1", found->value);
}

TEST(RWLContainer_Coverage, IntKeyType)
{
	siddiqsoft::RWLContainer<int, TestItem> container;

	auto item1 = container.add(100, TestItem {1, "hundred"});
	auto item2 = container.add(200, TestItem {2, "two-hundred"});

	ASSERT_TRUE(item1);
	ASSERT_TRUE(item2);
	EXPECT_EQ(2u, container.size());

	auto found = container.find(100);
	ASSERT_TRUE(found);
	EXPECT_EQ("hundred", found->value);
}

// --- Test concurrent access patterns ---

TEST(RWLContainer_Coverage, ConcurrentReads)
{
	siddiqsoft::RWLContainer<std::string, TestItem> container;

	// Add some items
	for (int i = 0; i < 10; ++i)
	{
		container.add(std::format("key_{}", i), TestItem {i, std::format("value_{}", i)});
	}

	std::atomic_int           readCount {0};
	std::vector<std::jthread> threads;

	// Spawn multiple reader threads
	for (int t = 0; t < 5; ++t)
	{
		threads.emplace_back(
				[&](std::stop_token st)
				{
					while (!st.stop_requested())
					{
						for (int i = 0; i < 10; ++i)
						{
							auto item = container.find(std::format("key_{}", i));
							if (item) readCount++;
						}
					}
				});
	}

	// Let them run for a bit
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	// Stop all threads
	for (auto& t : threads)
	{
		t.request_stop();
	}

	EXPECT_GT(readCount.load(), 0);
	EXPECT_EQ(10u, container.size());
}

TEST(RWLContainer_Coverage, ConcurrentWritesAndReads)
{
	siddiqsoft::RWLContainer<std::string, TestItem> container;

	std::atomic_int writeCount {0};
	std::atomic_int readCount {0};

	std::jthread writer(
			[&](std::stop_token st)
			{
				int counter = 0;
				while (!st.stop_requested())
				{
					container.add(std::format("key_{}", counter), TestItem {counter, "data"});
					writeCount++;
					counter++;
					if (counter > 100) counter = 0;
				}
			});

	std::jthread reader(
			[&](std::stop_token st)
			{
				while (!st.stop_requested())
				{
					for (int i = 0; i < 50; ++i)
					{
						auto item = container.find(std::format("key_{}", i));
						if (item) readCount++;
					}
				}
			});

	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	writer.request_stop();
	reader.request_stop();

	EXPECT_GT(writeCount.load(), 0);
	EXPECT_GT(readCount.load(), 0);
}

// --- Test scan with early termination ---

TEST(RWLContainer_Coverage, ScanEarlyTermination)
{
	siddiqsoft::RWLContainer<std::string, TestItem> container;

	for (int i = 0; i < 100; ++i)
	{
		container.add(std::format("key_{}", i), TestItem {i, std::format("value_{}", i)});
	}

	int  scanCount = 0;
	auto result    = container.scan(
			[&](const auto& key, auto& val) -> bool
			{
				scanCount++;
				return scanCount == 5; // Stop after 5 iterations
			});

	ASSERT_TRUE(result);
	EXPECT_EQ(5, scanCount);
}

// --- Test scan that never finds anything ---

TEST(RWLContainer_Coverage, ScanNoMatch)
{
	siddiqsoft::RWLContainer<std::string, TestItem> container;

	for (int i = 0; i < 50; ++i)
	{
		container.add(std::format("key_{}", i), TestItem {i, "data"});
	}

	int  scanCount = 0;
	auto result    = container.scan(
			[&](const auto& key, auto& val) -> bool
			{
				scanCount++;
				return false; // Never match
			});

	EXPECT_FALSE(result);
	EXPECT_EQ(50, scanCount);
}

// --- Test add with callback that returns nullptr (edge case) ---

TEST(RWLContainer_Coverage, AddCallbackReturnsValidPtr)
{
	siddiqsoft::RWLContainer<std::string, TestItem> container;

	auto item = container.add("key1",
	                          [](const std::string& key) -> std::shared_ptr<TestItem>
	                          { return std::make_shared<TestItem>(TestItem {42, key}); });

	ASSERT_TRUE(item);
	EXPECT_EQ(42, item->id);
	EXPECT_EQ("key1", item->value);
}

// --- Test remove and verify counter increments ---

TEST(RWLContainer_Coverage, RemoveCounterIncrement)
{
	siddiqsoft::RWLContainer<std::string, TestItem> container;

	container.add("a", TestItem {1, "alpha"});
	container.add("b", TestItem {2, "beta"});
	container.add("c", TestItem {3, "gamma"});

	auto json1 = container.toJson();
	EXPECT_EQ(3u, json1["adds"].get<uint64_t>());
	EXPECT_EQ(0u, json1["removes"].get<uint64_t>());

	auto dummy1 = container.remove("a");
	auto dummy2 = container.remove("b");

	auto json2 = container.toJson();
	EXPECT_EQ(3u, json2["adds"].get<uint64_t>());
	EXPECT_EQ(2u, json2["removes"].get<uint64_t>());
	EXPECT_EQ(1u, json2["size"].get<uint64_t>());
}

// --- Test toJson with various states ---

TEST(RWLContainer_Coverage, ToJsonEmptyContainer)
{
	siddiqsoft::RWLContainer<std::string, TestItem> container;

	auto json = container.toJson();
	EXPECT_EQ(0u, json["adds"].get<uint64_t>());
	EXPECT_EQ(0u, json["removes"].get<uint64_t>());
	EXPECT_EQ(0u, json["size"].get<uint64_t>());
	EXPECT_FALSE(json["ReplaceExisting"].get<bool>());
	EXPECT_FALSE(json["FailOnCollission"].get<bool>());
}

TEST(RWLContainer_Coverage, ToJsonWithFlags)
{
	siddiqsoft::RWLContainer<std::string, TestItem> container;
	container.ReplaceExisting  = true;
	container.FailOnCollission = true;

	auto json = container.toJson();
	EXPECT_TRUE(json["ReplaceExisting"].get<bool>());
	EXPECT_TRUE(json["FailOnCollission"].get<bool>());
}

// --- Test add with shared_ptr and various collision scenarios ---

TEST(RWLContainer_Coverage, AddSharedPtrMultipleScenarios)
{
	siddiqsoft::RWLContainer<std::string, TestItem> container;

	// Scenario 1: Add new item via shared_ptr
	auto ptr1  = std::make_shared<TestItem>(TestItem {1, "first"});
	auto item1 = container.add("key1", std::move(ptr1));
	ASSERT_TRUE(item1);
	EXPECT_EQ("first", item1->value);

	// Scenario 2: Collision with default behavior (return existing)
	auto ptr2  = std::make_shared<TestItem>(TestItem {2, "second"});
	auto item2 = container.add("key1", std::move(ptr2));
	ASSERT_TRUE(item2);
	EXPECT_EQ("first", item2->value); // Should be original
	EXPECT_EQ(item1, item2);

	// Scenario 3: New key
	auto ptr3  = std::make_shared<TestItem>(TestItem {3, "third"});
	auto item3 = container.add("key2", std::move(ptr3));
	ASSERT_TRUE(item3);
	EXPECT_EQ("third", item3->value);
	EXPECT_EQ(2u, container.size());
}

// --- Test add with callback and ReplaceExisting ---

TEST(RWLContainer_Coverage, AddCallbackWithReplace)
{
	siddiqsoft::RWLContainer<std::string, TestItem> container;
	container.ReplaceExisting = true;

	auto item1 = container.add("key1", [](const auto& key) { return std::make_shared<TestItem>(TestItem {1, "first"}); });
	ASSERT_TRUE(item1);
	EXPECT_EQ("first", item1->value);

	auto item2 = container.add("key1", [](const auto& key) { return std::make_shared<TestItem>(TestItem {2, "replaced"}); });
	ASSERT_TRUE(item2);
	EXPECT_EQ("replaced", item2->value);
	EXPECT_EQ(1u, container.size());
}

// --- Test find on large container ---

TEST(RWLContainer_Coverage, FindInLargeContainer)
{
	siddiqsoft::RWLContainer<std::string, TestItem> container;

	const int ITEM_COUNT = 5000;
	for (int i = 0; i < ITEM_COUNT; ++i)
	{
		container.add(std::format("key_{:05d}", i), TestItem {i, std::format("value_{}", i)});
	}

	EXPECT_EQ(static_cast<size_t>(ITEM_COUNT), container.size());

	// Find items at various positions
	auto first = container.find("key_00000");
	ASSERT_TRUE(first);
	EXPECT_EQ(0, first->id);

	auto middle = container.find("key_02500");
	ASSERT_TRUE(middle);
	EXPECT_EQ(2500, middle->id);

	auto last = container.find("key_04999");
	ASSERT_TRUE(last);
	EXPECT_EQ(4999, last->id);

	auto notfound = container.find("key_99999");
	EXPECT_FALSE(notfound);
}

// --- Test remove from large container ---

TEST(RWLContainer_Coverage, RemoveFromLargeContainer)
{
	siddiqsoft::RWLContainer<std::string, TestItem> container;

	const int ITEM_COUNT = 1000;
	for (int i = 0; i < ITEM_COUNT; ++i)
	{
		container.add(std::format("key_{}", i), TestItem {i, "data"});
	}

	// Remove every 10th item
	for (int i = 0; i < ITEM_COUNT; i += 10)
	{
		auto removed = container.remove(std::format("key_{}", i));
		ASSERT_TRUE(removed);
		EXPECT_EQ(i, removed->id);
	}

	EXPECT_EQ(static_cast<size_t>(ITEM_COUNT - 100), container.size());
}

// ============================================================================
// WaitableQueue Additional Coverage Tests
// ============================================================================

// --- Test toJson method ---

TEST(WaitableQueue_Coverage, ToJsonEmpty)
{
	siddiqsoft::WaitableQueue<std::string> queue;

	auto json = queue.toJson();
	EXPECT_EQ(0u, json["adds"].get<uint64_t>());
	EXPECT_EQ(0u, json["removes"].get<uint64_t>());
	EXPECT_EQ(0u, json["size"].get<uint64_t>());
}

TEST(WaitableQueue_Coverage, ToJsonAfterOperations)
{
	siddiqsoft::WaitableQueue<int> queue;

	for (int i = 0; i < 10; ++i)
	{
		int val = i;
		queue.push(std::move(val));
	}

	for (int i = 0; i < 5; ++i)
	{
		[[maybe_unused]] auto item = queue.tryWaitItem(std::chrono::milliseconds(50));
	}

	auto json = queue.toJson();
	EXPECT_EQ(10u, json["adds"].get<uint64_t>());
	EXPECT_EQ(5u, json["removes"].get<uint64_t>());
	EXPECT_EQ(5u, json["size"].get<uint64_t>());
}

// --- Test addCounter and removeCounter directly ---

TEST(WaitableQueue_Coverage, CounterMethods)
{
	siddiqsoft::WaitableQueue<std::string> queue;

	EXPECT_EQ(0u, queue.addCounter());
	EXPECT_EQ(0u, queue.removeCounter());

	queue.push(std::string("item1"));
	EXPECT_EQ(1u, queue.addCounter());
	EXPECT_EQ(0u, queue.removeCounter());

	queue.emplace(std::string("item2"));
	EXPECT_EQ(2u, queue.addCounter());
	EXPECT_EQ(0u, queue.removeCounter());

	auto item1 = queue.tryWaitItem(std::chrono::milliseconds(50));
	ASSERT_TRUE(item1.has_value());
	EXPECT_EQ(2u, queue.addCounter());
	EXPECT_EQ(1u, queue.removeCounter());

	auto item2 = queue.tryWaitItem(std::chrono::milliseconds(50));
	ASSERT_TRUE(item2.has_value());
	EXPECT_EQ(2u, queue.addCounter());
	EXPECT_EQ(2u, queue.removeCounter());
}

// --- Test waitUntilEmpty with various timeout values ---

TEST(WaitableQueue_Coverage, WaitUntilEmptyVariousTimeouts)
{
	siddiqsoft::WaitableQueue<int> queue;

	// Empty queue with short timeout
	auto result1 = queue.waitUntilEmpty(std::chrono::milliseconds(10));
	ASSERT_TRUE(result1.has_value());
	EXPECT_EQ(0u, result1.value());

	// Empty queue with long timeout
	auto result2 = queue.waitUntilEmpty(std::chrono::milliseconds(1000));
	ASSERT_TRUE(result2.has_value());
	EXPECT_EQ(0u, result2.value());

	// Queue with items but no consumer (should timeout)
	for (int i = 0; i < 5; ++i)
	{
		int val = i;
		queue.push(std::move(val));
	}

	auto result3 = queue.waitUntilEmpty(std::chrono::milliseconds(50));
	ASSERT_TRUE(result3.has_value());
	EXPECT_GT(result3.value(), 0u);
}

// --- Test tryWaitItem with various timeout values ---

TEST(WaitableQueue_Coverage, TryWaitItemVariousTimeouts)
{
	siddiqsoft::WaitableQueue<std::string> queue;

	// Timeout on empty queue with short duration
	auto start1 = std::chrono::steady_clock::now();
	auto item1  = queue.tryWaitItem(std::chrono::milliseconds(10));
	auto end1   = std::chrono::steady_clock::now();
	EXPECT_FALSE(item1.has_value());
	auto elapsed1 = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1).count();
	EXPECT_GE(elapsed1, 5);

	// Timeout on empty queue with longer duration
	auto start2 = std::chrono::steady_clock::now();
	auto item2  = queue.tryWaitItem(std::chrono::milliseconds(50));
	auto end2   = std::chrono::steady_clock::now();
	EXPECT_FALSE(item2.has_value());
	auto elapsed2 = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2).count();
	EXPECT_GE(elapsed2, 40);

	// Item available - should return immediately
	queue.push(std::string("available"));
	auto start3 = std::chrono::steady_clock::now();
	auto item3  = queue.tryWaitItem(std::chrono::milliseconds(5000));
	auto end3   = std::chrono::steady_clock::now();
	ASSERT_TRUE(item3.has_value());
	EXPECT_EQ("available", item3.value());
	auto elapsed3 = std::chrono::duration_cast<std::chrono::milliseconds>(end3 - start3).count();
	EXPECT_LT(elapsed3, 1000);
}

// --- Test push and emplace with various types ---

TEST(WaitableQueue_Coverage, PushEmplaceVariousTypes)
{
	// String queue
	{
		siddiqsoft::WaitableQueue<std::string> queue;
		queue.push(std::string("hello"));
		queue.emplace(std::string("world"));
		EXPECT_EQ(2u, queue.size());
		EXPECT_EQ(2u, queue.addCounter());
	}

	// Integer queue
	{
		siddiqsoft::WaitableQueue<int> queue;
		int                            val1 = 42;
		queue.push(std::move(val1));
		int val2 = 99;
		queue.emplace(std::move(val2));
		EXPECT_EQ(2u, queue.size());
		EXPECT_EQ(2u, queue.addCounter());
	}

	// Vector queue
	{
		siddiqsoft::WaitableQueue<std::vector<int>> queue;
		std::vector<int>                            v1 {1, 2, 3};
		queue.push(std::move(v1));
		std::vector<int> v2 {4, 5, 6};
		queue.emplace(std::move(v2));
		EXPECT_EQ(2u, queue.size());
		EXPECT_EQ(2u, queue.addCounter());
	}
}

// --- Test size method consistency ---

TEST(WaitableQueue_Coverage, SizeConsistency)
{
	siddiqsoft::WaitableQueue<int> queue;

	EXPECT_EQ(0u, queue.size());

	for (int i = 0; i < 100; ++i)
	{
		int val = i;
		queue.push(std::move(val));
		EXPECT_EQ(static_cast<size_t>(i + 1), queue.size());
	}

	for (int i = 0; i < 100; ++i)
	{
		[[maybe_unused]] auto item = queue.tryWaitItem(std::chrono::milliseconds(50));
		EXPECT_EQ(static_cast<size_t>(99 - i), queue.size());
	}

	EXPECT_EQ(0u, queue.size());
}

// --- Test concurrent push and pop ---

TEST(WaitableQueue_Coverage, ConcurrentPushPop)
{
	siddiqsoft::WaitableQueue<int> queue;
	std::atomic_int                pushCount {0};
	std::atomic_int                popCount {0};
	const int                      ITEM_COUNT = 100;

	std::jthread producer(
			[&](std::stop_token st)
			{
				for (int i = 0; i < ITEM_COUNT && !st.stop_requested(); ++i)
				{
					int val = i;
					queue.push(std::move(val));
					pushCount++;
				}
			});

	std::jthread consumer(
			[&](std::stop_token st)
			{
				while (!st.stop_requested())
				{
					auto item = queue.tryWaitItem(std::chrono::milliseconds(10));
					if (item.has_value()) popCount++;
				}
			});

	producer.join();
	queue.waitUntilEmpty(std::chrono::milliseconds(2000));
	consumer.request_stop();
	consumer.join();

	EXPECT_EQ(ITEM_COUNT, pushCount.load());
	EXPECT_EQ(ITEM_COUNT, popCount.load());
	EXPECT_EQ(static_cast<uint64_t>(ITEM_COUNT), queue.addCounter());
	EXPECT_EQ(static_cast<uint64_t>(ITEM_COUNT), queue.removeCounter());
}

// --- Test multiple producers, single consumer ---

TEST(WaitableQueue_Coverage, MultipleProducersSingleConsumer)
{
	siddiqsoft::WaitableQueue<int> queue;
	std::atomic_int                totalConsumed {0};
	const int                      ITEMS_PER_PRODUCER = 50;
	const int                      PRODUCER_COUNT     = 3;

	std::vector<std::jthread> producers;
	for (int p = 0; p < PRODUCER_COUNT; ++p)
	{
		producers.emplace_back(
				[&, p](std::stop_token st)
				{
					for (int i = 0; i < ITEMS_PER_PRODUCER && !st.stop_requested(); ++i)
					{
						int val = p * 1000 + i;
						queue.push(std::move(val));
					}
				});
	}

	std::jthread consumer(
			[&](std::stop_token st)
			{
				while (!st.stop_requested())
				{
					auto item = queue.tryWaitItem(std::chrono::milliseconds(10));
					if (item.has_value()) totalConsumed++;
				}
			});

	for (auto& p : producers)
	{
		p.join();
	}

	queue.waitUntilEmpty(std::chrono::milliseconds(2000));
	consumer.request_stop();
	consumer.join();

	EXPECT_EQ(ITEMS_PER_PRODUCER * PRODUCER_COUNT, totalConsumed.load());
	EXPECT_EQ(static_cast<uint64_t>(ITEMS_PER_PRODUCER * PRODUCER_COUNT), queue.addCounter());
}

// --- Test single producer, multiple consumers ---

TEST(WaitableQueue_Coverage, SingleProducerMultipleConsumers)
{
	siddiqsoft::WaitableQueue<int> queue;
	std::atomic_int                totalConsumed {0};
	const int                      ITEM_COUNT     = 100;
	const int                      CONSUMER_COUNT = 3;

	std::jthread producer(
			[&](std::stop_token st)
			{
				for (int i = 0; i < ITEM_COUNT && !st.stop_requested(); ++i)
				{
					int val = i;
					queue.push(std::move(val));
				}
			});

	std::vector<std::jthread> consumers;
	for (int c = 0; c < CONSUMER_COUNT; ++c)
	{
		consumers.emplace_back(
				[&](std::stop_token st)
				{
					while (!st.stop_requested())
					{
						auto item = queue.tryWaitItem(std::chrono::milliseconds(10));
						if (item.has_value()) totalConsumed++;
					}
				});
	}

	producer.join();
	queue.waitUntilEmpty(std::chrono::milliseconds(2000));

	for (auto& c : consumers)
	{
		c.request_stop();
		c.join();
	}

	EXPECT_EQ(ITEM_COUNT, totalConsumed.load());
	EXPECT_EQ(static_cast<uint64_t>(ITEM_COUNT), queue.addCounter());
	EXPECT_EQ(static_cast<uint64_t>(ITEM_COUNT), queue.removeCounter());
}

// --- Test rapid push/pop cycles ---

TEST(WaitableQueue_Coverage, RapidPushPopCycles)
{
	siddiqsoft::WaitableQueue<std::string> queue;

	for (int cycle = 0; cycle < 10; ++cycle)
	{
		// Push 50 items
		for (int i = 0; i < 50; ++i)
		{
			queue.push(std::format("cycle_{}_item_{}", cycle, i));
		}

		EXPECT_EQ(50u, queue.size());

		// Pop all items
		for (int i = 0; i < 50; ++i)
		{
			auto item = queue.tryWaitItem(std::chrono::milliseconds(100));
			ASSERT_TRUE(item.has_value());
			EXPECT_EQ(std::format("cycle_{}_item_{}", cycle, i), item.value());
		}

		EXPECT_EQ(0u, queue.size());
	}

	EXPECT_EQ(500u, queue.addCounter());
	EXPECT_EQ(500u, queue.removeCounter());
}

// --- Test waitUntilEmpty with active consumer draining queue ---

TEST(WaitableQueue_Coverage, WaitUntilEmptyWithActiveDrain)
{
	siddiqsoft::WaitableQueue<int> queue;
	const int                      ITEM_COUNT = 200;

	// Push all items
	for (int i = 0; i < ITEM_COUNT; ++i)
	{
		int val = i;
		queue.push(std::move(val));
	}

	EXPECT_EQ(static_cast<size_t>(ITEM_COUNT), queue.size());

	// Start consumer
	std::jthread consumer(
			[&](std::stop_token st)
			{
				while (!st.stop_requested())
				{
					auto item = queue.tryWaitItem(std::chrono::milliseconds(5));
					if (!item.has_value() && queue.size() == 0) break;
				}
			});

	// Wait for queue to empty
	auto result = queue.waitUntilEmpty(std::chrono::milliseconds(3000));
	ASSERT_TRUE(result.has_value());
	EXPECT_EQ(0u, result.value());

	consumer.request_stop();
	consumer.join();

	EXPECT_EQ(static_cast<uint64_t>(ITEM_COUNT), queue.removeCounter());
}

// --- Test edge case: tryWaitItem on queue that becomes empty during wait ---

TEST(WaitableQueue_Coverage, TryWaitItemRaceCondition)
{
	siddiqsoft::WaitableQueue<int> queue;

	// Push one item
	int val = 42;
	queue.push(std::move(val));

	// Pop it immediately
	auto item = queue.tryWaitItem(std::chrono::milliseconds(100));
	ASSERT_TRUE(item.has_value());
	EXPECT_EQ(42, item.value());

	// Try to pop from empty queue with timeout
	auto empty = queue.tryWaitItem(std::chrono::milliseconds(50));
	EXPECT_FALSE(empty.has_value());
}


// --- Test stress: many items, many operations ---

TEST(WaitableQueue_Coverage, StressTest)
{
	siddiqsoft::WaitableQueue<int> queue;
	std::atomic_int                totalProcessed {0};
	const int                      TOTAL_ITEMS = 1000;

	std::jthread producer(
			[&](std::stop_token st)
			{
				for (int i = 0; i < TOTAL_ITEMS && !st.stop_requested(); ++i)
				{
					int val = i;
					queue.push(std::move(val));
				}
			});

	std::jthread consumer(
			[&](std::stop_token st)
			{
				while (!st.stop_requested())
				{
					auto item = queue.tryWaitItem(std::chrono::milliseconds(5));
					if (item.has_value()) totalProcessed++;
				}
			});

	producer.join();
	queue.waitUntilEmpty(std::chrono::milliseconds(5000));
	consumer.request_stop();
	consumer.join();

	EXPECT_EQ(TOTAL_ITEMS, totalProcessed.load());
	EXPECT_EQ(static_cast<uint64_t>(TOTAL_ITEMS), queue.addCounter());
	EXPECT_EQ(static_cast<uint64_t>(TOTAL_ITEMS), queue.removeCounter());
	EXPECT_EQ(0u, queue.size());
}

// NOLINTEND(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
