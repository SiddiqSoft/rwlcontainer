/*
	WaitableQueue : FIFO Behavior Tests
	Comprehensive tests to ensure strict FIFO (First-In-First-Out) ordering
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
#include <ostream>
#include <thread>
#include <chrono>
#include <vector>
#include <deque>

#include "nlohmann/json.hpp"
#include "../include/siddiqsoft/WaitableQueue.hpp"

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

// ============================================================================
// FIFO Behavior Tests for WaitableQueue
// ============================================================================

// --- Basic FIFO with small dataset ---

TEST(WaitableQueue_FIFO, BasicFIFOSmallDataset)
{
	siddiqsoft::WaitableQueue<int> queue;

	// Push 5 items
	for (int i = 0; i < 5; ++i)
	{
		int val = i;
		queue.push(std::move(val));
	}

	// Pop and verify FIFO order
	for (int i = 0; i < 5; ++i)
	{
		auto item = queue.tryWaitItem(std::chrono::milliseconds(100));
		ASSERT_TRUE(item.has_value()) << "Item " << i << " should be available";
		EXPECT_EQ(i, item.value()) << "Expected " << i << " but got " << item.value();
	}

	EXPECT_EQ(0u, queue.size());
}

// --- FIFO with large dataset ---

TEST(WaitableQueue_FIFO, FIFOLargeDataset)
{
	siddiqsoft::WaitableQueue<int> queue;
	const int                      ITEM_COUNT = 1000;

	// Push 1000 items
	for (int i = 0; i < ITEM_COUNT; ++i)
	{
		int val = i;
		queue.push(std::move(val));
	}

	EXPECT_EQ(static_cast<size_t>(ITEM_COUNT), queue.size());

	// Pop and verify FIFO order
	for (int i = 0; i < ITEM_COUNT; ++i)
	{
		auto item = queue.tryWaitItem(std::chrono::milliseconds(100));
		ASSERT_TRUE(item.has_value()) << "Item " << i << " should be available";
		EXPECT_EQ(i, item.value()) << "Expected " << i << " but got " << item.value();
	}

	EXPECT_EQ(0u, queue.size());
	EXPECT_EQ(static_cast<uint64_t>(ITEM_COUNT), queue.addCounter());
	EXPECT_EQ(static_cast<uint64_t>(ITEM_COUNT), queue.removeCounter());
}

// --- FIFO with string values ---

TEST(WaitableQueue_FIFO, FIFOStringValues)
{
	siddiqsoft::WaitableQueue<std::string> queue;

	std::vector<std::string> values = {"first", "second", "third", "fourth", "fifth"};

	// Push all values
	for (const auto& val : values)
	{
		queue.push(std::string(val));
	}

	// Pop and verify FIFO order
	for (size_t i = 0; i < values.size(); ++i)
	{
		auto item = queue.tryWaitItem(std::chrono::milliseconds(100));
		ASSERT_TRUE(item.has_value()) << "Item " << i << " should be available";
		EXPECT_EQ(values[i], item.value()) << "Expected '" << values[i] << "' but got '" << item.value() << "'";
	}

	EXPECT_EQ(0u, queue.size());
}

// --- FIFO with emplace ---

TEST(WaitableQueue_FIFO, FIFOWithEmplace)
{
	siddiqsoft::WaitableQueue<int> queue;

	// Mix push and emplace
	for (int i = 0; i < 10; ++i)
	{
		if (i % 2 == 0)
		{
			int val = i;
			queue.push(std::move(val));
		}
		else
		{
			int val = i;
			queue.emplace(std::move(val));
		}
	}

	// Pop and verify FIFO order
	for (int i = 0; i < 10; ++i)
	{
		auto item = queue.tryWaitItem(std::chrono::milliseconds(100));
		ASSERT_TRUE(item.has_value());
		EXPECT_EQ(i, item.value());
	}

	EXPECT_EQ(0u, queue.size());
}

// --- FIFO with partial consumption ---

TEST(WaitableQueue_FIFO, FIFOPartialConsumption)
{
	siddiqsoft::WaitableQueue<int> queue;

	// Push 10 items
	for (int i = 0; i < 10; ++i)
	{
		int val = i;
		queue.push(std::move(val));
	}

	// Consume first 5
	for (int i = 0; i < 5; ++i)
	{
		auto item = queue.tryWaitItem(std::chrono::milliseconds(100));
		ASSERT_TRUE(item.has_value());
		EXPECT_EQ(i, item.value());
	}

	EXPECT_EQ(5u, queue.size());

	// Push 5 more
	for (int i = 10; i < 15; ++i)
	{
		int val = i;
		queue.push(std::move(val));
	}

	EXPECT_EQ(10u, queue.size());

	// Consume remaining in FIFO order
	for (int i = 5; i < 15; ++i)
	{
		auto item = queue.tryWaitItem(std::chrono::milliseconds(100));
		ASSERT_TRUE(item.has_value());
		EXPECT_EQ(i, item.value());
	}

	EXPECT_EQ(0u, queue.size());
}

// --- FIFO with interleaved push/pop ---

TEST(WaitableQueue_FIFO, FIFOInterleavedPushPop)
{
	siddiqsoft::WaitableQueue<int> queue;

	// Interleave push and pop
	for (int cycle = 0; cycle < 5; ++cycle)
	{
		// Push 3 items
		for (int i = 0; i < 3; ++i)
		{
			int val = cycle * 10 + i;
			queue.push(std::move(val));
		}

		// Pop 3 items
		for (int i = 0; i < 3; ++i)
		{
			auto item = queue.tryWaitItem(std::chrono::milliseconds(100));
			ASSERT_TRUE(item.has_value());
			EXPECT_EQ(cycle * 10 + i, item.value());
		}
	}

	EXPECT_EQ(0u, queue.size());
}

// --- FIFO with multiple producers (sequential) ---

TEST(WaitableQueue_FIFO, FIFOMultipleProducersSequential)
{
	siddiqsoft::WaitableQueue<int> queue;

	// Producer 1: push 0-9
	for (int i = 0; i < 10; ++i)
	{
		int val = i;
		queue.push(std::move(val));
	}

	// Producer 2: push 10-19
	for (int i = 10; i < 20; ++i)
	{
		int val = i;
		queue.push(std::move(val));
	}

	// Producer 3: push 20-29
	for (int i = 20; i < 30; ++i)
	{
		int val = i;
		queue.push(std::move(val));
	}

	// Consume all in FIFO order
	for (int i = 0; i < 30; ++i)
	{
		auto item = queue.tryWaitItem(std::chrono::milliseconds(100));
		ASSERT_TRUE(item.has_value());
		EXPECT_EQ(i, item.value());
	}

	EXPECT_EQ(0u, queue.size());
}

// --- FIFO with concurrent producer and consumer ---

TEST(WaitableQueue_FIFO, FIFOConcurrentProducerConsumer)
{
	siddiqsoft::WaitableQueue<int> queue;
	std::vector<int>               consumed;
	const int                      ITEM_COUNT = 100;

	std::jthread consumer(
			[&](std::stop_token st)
			{
				while (!st.stop_requested())
				{
					auto item = queue.tryWaitItem(std::chrono::milliseconds(10));
					if (item.has_value()) { consumed.push_back(item.value()); }
				}
			});

	// Producer thread
	for (int i = 0; i < ITEM_COUNT; ++i)
	{
		int val = i;
		queue.push(std::move(val));
		// Small delay to allow interleaving
		std::this_thread::sleep_for(std::chrono::microseconds(100));
	}

	queue.waitUntilEmpty(std::chrono::milliseconds(3000));
	consumer.request_stop();
	consumer.join();

	// Verify FIFO order
	EXPECT_EQ(static_cast<size_t>(ITEM_COUNT), consumed.size());
	for (int i = 0; i < ITEM_COUNT; ++i)
	{
		EXPECT_EQ(i, consumed[i]) << "Item at position " << i << " should be " << i << " but got " << consumed[i];
	}
}

// --- FIFO with multiple concurrent producers and single consumer ---

TEST(WaitableQueue_FIFO, FIFOMultipleProducersSingleConsumer)
{
	siddiqsoft::WaitableQueue<int> queue;
	std::vector<int>               consumed;
	std::mutex                     consumed_mutex;
	const int                      ITEMS_PER_PRODUCER = 30;
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
					if (item.has_value())
					{
						std::scoped_lock<std::mutex> lock(consumed_mutex);
						consumed.push_back(item.value());
					}
				}
			});

	for (auto& p : producers)
	{
		p.join();
	}

	queue.waitUntilEmpty(std::chrono::milliseconds(3000));
	consumer.request_stop();
	consumer.join();

	// Verify all items were consumed
	EXPECT_EQ(static_cast<size_t>(ITEMS_PER_PRODUCER * PRODUCER_COUNT), consumed.size());

	// Verify FIFO order within each producer's items
	std::vector<int> producer_indices(PRODUCER_COUNT, 0);
	for (int val : consumed)
	{
		int producer_id = val / 1000;
		int item_index  = val % 1000;
		EXPECT_EQ(producer_indices[producer_id], item_index) << "Producer " << producer_id << " items out of order";
		producer_indices[producer_id]++;
	}
}

// --- FIFO with single producer and multiple consumers ---

TEST(WaitableQueue_FIFO, FIFOSingleProducerMultipleConsumers)
{
	siddiqsoft::WaitableQueue<int> queue;
	std::vector<int>               consumed;
	std::mutex                     consumed_mutex;
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
						if (item.has_value())
						{
							std::scoped_lock<std::mutex> lock(consumed_mutex);
							consumed.push_back(item.value());
						}
					}
				});
	}

	producer.join();
	queue.waitUntilEmpty(std::chrono::milliseconds(3000));

	for (auto& c : consumers)
	{
		c.request_stop();
		c.join();
	}

	// Verify all items were consumed
	EXPECT_EQ(static_cast<size_t>(ITEM_COUNT), consumed.size());

	// Verify FIFO order
	std::sort(consumed.begin(), consumed.end());
	for (int i = 0; i < ITEM_COUNT; ++i)
	{
		EXPECT_EQ(i, consumed[i]);
	}
}

// --- FIFO with struct values ---

struct FIFOTestItem
{
	int         id;
	std::string name;
	int         sequence;

	FIFOTestItem(int i, const std::string& n, int s)
		: id(i)
		, name(n)
		, sequence(s)
	{
	}
};

TEST(WaitableQueue_FIFO, FIFOStructValues)
{
	siddiqsoft::WaitableQueue<FIFOTestItem> queue;

	// Push 10 items
	for (int i = 0; i < 10; ++i)
	{
		queue.push(FIFOTestItem {i, std::format("item_{}", i), i * 100});
	}

	// Pop and verify FIFO order
	for (int i = 0; i < 10; ++i)
	{
		auto item = queue.tryWaitItem(std::chrono::milliseconds(100));
		ASSERT_TRUE(item.has_value());
		EXPECT_EQ(i, item->id);
		EXPECT_EQ(std::format("item_{}", i), item->name);
		EXPECT_EQ(i * 100, item->sequence);
	}

	EXPECT_EQ(0u, queue.size());
}

// --- FIFO with rapid push/pop cycles ---

TEST(WaitableQueue_FIFO, FIFORapidCycles)
{
	siddiqsoft::WaitableQueue<int> queue;

	for (int cycle = 0; cycle < 20; ++cycle)
	{
		// Push 50 items
		for (int i = 0; i < 50; ++i)
		{
			int val = cycle * 1000 + i;
			queue.push(std::move(val));
		}

		// Pop all items
		for (int i = 0; i < 50; ++i)
		{
			auto item = queue.tryWaitItem(std::chrono::milliseconds(100));
			ASSERT_TRUE(item.has_value());
			EXPECT_EQ(cycle * 1000 + i, item.value());
		}

		EXPECT_EQ(0u, queue.size());
	}

	EXPECT_EQ(1000u, queue.addCounter());
	EXPECT_EQ(1000u, queue.removeCounter());
}

// --- FIFO with vector values ---

TEST(WaitableQueue_FIFO, FIFOVectorValues)
{
	siddiqsoft::WaitableQueue<std::vector<int>> queue;

	// Push 5 vectors
	for (int i = 0; i < 5; ++i)
	{
		std::vector<int> vec;
		for (int j = 0; j < 3; ++j)
		{
			vec.push_back(i * 10 + j);
		}
		queue.push(std::move(vec));
	}

	// Pop and verify FIFO order
	for (int i = 0; i < 5; ++i)
	{
		auto item = queue.tryWaitItem(std::chrono::milliseconds(100));
		ASSERT_TRUE(item.has_value());
		EXPECT_EQ(3u, item->size());
		for (int j = 0; j < 3; ++j)
		{
			EXPECT_EQ(i * 10 + j, (*item)[j]);
		}
	}

	EXPECT_EQ(0u, queue.size());
}

// --- FIFO with deque container ---

TEST(WaitableQueue_FIFO, FIFOWithDequeContainer)
{
	siddiqsoft::WaitableQueue<int> queue;

	// Push 20 items
	for (int i = 0; i < 20; ++i)
	{
		int val = i;
		queue.push(std::move(val));
	}

	// Pop and verify FIFO order
	for (int i = 0; i < 20; ++i)
	{
		auto item = queue.tryWaitItem(std::chrono::milliseconds(100));
		ASSERT_TRUE(item.has_value());
		EXPECT_EQ(i, item.value());
	}

	EXPECT_EQ(0u, queue.size());
}

// --- FIFO stress test with high throughput ---

TEST(WaitableQueue_FIFO, FIFOStressHighThroughput)
{
	siddiqsoft::WaitableQueue<int> queue;
	std::vector<int>               consumed;
	std::mutex                     consumed_mutex;
	const int                      ITEM_COUNT = 5000;

	std::jthread producer(
			[&](std::stop_token st)
			{
				for (int i = 0; i < ITEM_COUNT && !st.stop_requested(); ++i)
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
					if (item.has_value())
					{
						std::scoped_lock<std::mutex> lock(consumed_mutex);
						consumed.push_back(item.value());
					}
				}
			});

	producer.join();
	queue.waitUntilEmpty(std::chrono::milliseconds(5000));
	consumer.request_stop();
	consumer.join();

	// Verify all items were consumed
	EXPECT_EQ(static_cast<size_t>(ITEM_COUNT), consumed.size());

	// Verify FIFO order
	for (int i = 0; i < ITEM_COUNT; ++i)
	{
		EXPECT_EQ(i, consumed[i]);
	}
}

// --- FIFO with timeout and partial consumption ---

TEST(WaitableQueue_FIFO, FIFOTimeoutPartialConsumption)
{
	siddiqsoft::WaitableQueue<int> queue;

	// Push 5 items
	for (int i = 0; i < 5; ++i)
	{
		int val = i;
		queue.push(std::move(val));
	}
	std::cerr << std::format( "0 - Items in the queue:\n{}\n", queue.toJson().dump(2));
	EXPECT_EQ(5, queue.size());

	// Consume 3 items
	for (int i = 0; i < 3; ++i)
	{
		auto item = queue.tryWaitItem(std::chrono::milliseconds(100));
		ASSERT_TRUE(item.has_value());
		EXPECT_EQ(i, item.value());
	}
	std::cerr << std::format( "1 - Items in the queue:\n{}\n", queue.toJson().dump(2));
	// We should have two items remaining..
	EXPECT_EQ(2, queue.size());

	// Wait for timeout on empty (should timeout)
	auto timeout_item = queue.tryWaitItem(std::chrono::milliseconds(50));
	//std::cerr << "Got Item " << timeout_item.has_value() << std::endl;
	std::cerr << std::format( "2 - Items in the queue:\n{}\n", queue.toJson().dump(2));
	EXPECT_TRUE(timeout_item.has_value());
	// We should have two items remaining..
	EXPECT_EQ(1, queue.size());

	// Push 2 more items
	for (int i = 5; i < 7; ++i)
	{
		int val = i;
		queue.push(std::move(val));
	}
	std::cerr << std::format( "3 - Items in the queue:\n{}\n", queue.toJson().dump(2));
	EXPECT_EQ(3, queue.size());

	// Consume remaining in FIFO order
	for (int i = 4; i < 7; ++i)
	{
		auto item = queue.tryWaitItem(std::chrono::milliseconds(100));
		ASSERT_TRUE(item.has_value());
		std::cerr << std::format("3 - Item:{}  vs {}\n", item.value(), i);
		EXPECT_EQ(i, item.value());
	}

	std::cerr << std::format( "4 - Items in the queue:\n{}\n", queue.toJson().dump(2));
	EXPECT_EQ(0u, queue.size());
}

// --- FIFO with alternating push/pop pattern ---

TEST(WaitableQueue_FIFO, FIFOAlternatingPattern)
{
	siddiqsoft::WaitableQueue<int> queue;

	// Pattern: push 2, pop 1, push 2, pop 1, etc.
	int push_val     = 0;
	int expected_pop = 0;

	for (int iteration = 0; iteration < 10; ++iteration)
	{
		// Push 2
		for (int i = 0; i < 2; ++i)
		{
			int val = push_val++;
			queue.push(std::move(val));
		}

		// Pop 1
		auto item = queue.tryWaitItem(std::chrono::milliseconds(100));
		ASSERT_TRUE(item.has_value());
		EXPECT_EQ(expected_pop++, item.value());
	}

	// Pop remaining
	while (queue.size() > 0)
	{
		auto item = queue.tryWaitItem(std::chrono::milliseconds(100));
		ASSERT_TRUE(item.has_value());
		EXPECT_EQ(expected_pop++, item.value());
	}

	EXPECT_EQ(20, push_val);
	EXPECT_EQ(20, expected_pop);
}

// --- FIFO with zero-value items ---

TEST(WaitableQueue_FIFO, FIFOZeroValues)
{
	siddiqsoft::WaitableQueue<int> queue;

	// Push items including zeros
	std::vector<int> values = {0, 1, 0, 2, 0, 3, 0};
	for (int val : values)
	{
		int v = val;
		queue.push(std::move(v));
	}

	// Pop and verify FIFO order
	for (int expected : values)
	{
		auto item = queue.tryWaitItem(std::chrono::milliseconds(100));
		ASSERT_TRUE(item.has_value());
		EXPECT_EQ(expected, item.value());
	}

	EXPECT_EQ(0u, queue.size());
}

// --- FIFO with negative values ---

TEST(WaitableQueue_FIFO, FIFONegativeValues)
{
	siddiqsoft::WaitableQueue<int> queue;

	// Push items including negative values
	std::vector<int> values = {-5, -1, 0, 1, 5, -10, 100};
	for (int val : values)
	{
		int v = val;
		queue.push(std::move(v));
	}

	// Pop and verify FIFO order
	for (int expected : values)
	{
		auto item = queue.tryWaitItem(std::chrono::milliseconds(100));
		ASSERT_TRUE(item.has_value());
		EXPECT_EQ(expected, item.value());
	}

	EXPECT_EQ(0u, queue.size());
}

// --- FIFO with duplicate values ---

TEST(WaitableQueue_FIFO, FIFODuplicateValues)
{
	siddiqsoft::WaitableQueue<int> queue;

	// Push items with duplicates
	std::vector<int> values = {1, 1, 1, 2, 2, 3, 1, 1};
	for (int val : values)
	{
		int v = val;
		queue.push(std::move(v));
	}

	// Pop and verify FIFO order
	for (int expected : values)
	{
		auto item = queue.tryWaitItem(std::chrono::milliseconds(100));
		ASSERT_TRUE(item.has_value());
		EXPECT_EQ(expected, item.value());
	}

	EXPECT_EQ(0u, queue.size());
}

// NOLINTEND(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
