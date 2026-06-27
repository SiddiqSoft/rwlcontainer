
#include "gtest/gtest.h"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <format>
#include <functional>
#include <queue>
#include <string>
#include <unordered_map>
#include <thread>
#include <array>
#include <optional>
#include <format>

#include "../include/siddiqsoft/RWLContainer.hpp"
#include "../include/siddiqsoft/WaitableQueue.hpp"

static std::atomic_uint64_t CountObjectsDestroyed {0};

struct MyTestObject
{
	int                        id;
	std::string                name;
	std::optional<std::string> description;

public:
	MyTestObject(const std::string& n, const std::string& d = {})
		: name(n)
	{
		if (!d.empty()) description = d;
	}

	~MyTestObject()
	{
		CountObjectsDestroyed++;
#if defined(DEBUG) || defined(_DEBUG)
		std::cout << std::format("Destroyed >>{},{}<< {} Counter:{}\n",
		                         id,
		                         name,
		                         (description.has_value() ? *description : ""),
		                         CountObjectsDestroyed.load());
#endif
	}
};


// NOLINTBEGIN(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

TEST(WaitableQueueTests, CustomObject)
{
	siddiqsoft::WaitableQueue<MyTestObject> myContainer1;
	myContainer1.push(MyTestObject {"my name"});
}

TEST(WaitableQueueTests, LoadTest_1)
{
	CountObjectsDestroyed                                  = 0;
	static const auto                      ITERATION_COUNT = 910000;
	static const int                       THREAD_COUNT    = 4;
	siddiqsoft::WaitableQueue<std::string> myContainer;

	try
	{
		// The worker function for each thread..
		auto workerFunction = [](std::stop_token st, siddiqsoft::WaitableQueue<std::string>& myContainer)
		{
			uint64_t itemCount = 0;

			std::cout << std::this_thread::get_id() << " - LoadTest_1 Worker started." << std::endl;
			while (!st.stop_requested())
			{
				auto item = myContainer.tryWaitItem();
				if (item.has_value()) itemCount++;
			}
			std::cout << std::this_thread::get_id() << " - LoadTest_1 Worker ended..." << itemCount << std::endl;
		};

		// Create the workers..
		std::array<std::jthread, THREAD_COUNT> threadPool {};
		for (int i = 0; i < THREAD_COUNT; i++)
		{
			threadPool[i] = std::jthread(workerFunction, std::ref(myContainer));
		}

		for (auto i = 0; i < ITERATION_COUNT; i++)
		{
			myContainer.emplace(std::format("Item---------------------------: {}", i));
			if (i > (ITERATION_COUNT / THREAD_COUNT)) threadPool[THREAD_COUNT - 1].request_stop();
			if (i > (ITERATION_COUNT / THREAD_COUNT - 1)) threadPool[THREAD_COUNT - 2].request_stop();
			if (i > (ITERATION_COUNT / THREAD_COUNT - 2)) threadPool[THREAD_COUNT - 3].request_stop();
		}

		// Immediately request stop..
		for (auto& t : threadPool)
		{
			std::cout << "Force stopping thread #" << t.get_id() << std::endl;
			t.request_stop();
		}
	}
	catch (...)
	{
	}

	std::cout << std::format("{} - Expected Iteration count: {}  Adds: {}  Destroyed: {}\n",
	                         __func__,
	                         ITERATION_COUNT,
	                         myContainer.addCounter(),
	                         CountObjectsDestroyed.load());
	EXPECT_EQ(ITERATION_COUNT, myContainer.addCounter()) << myContainer.size();
}

TEST(WaitableQueueTests, LoadTest_2)
{
	CountObjectsDestroyed                                   = 0;
	static const auto                       ITERATION_COUNT = 10;
	static const int                        THREAD_COUNT    = 4;
	siddiqsoft::WaitableQueue<MyTestObject> myContainer;

	try
	{
		// The worker function for each thread..
		auto workerFunction = [](std::stop_token st, siddiqsoft::WaitableQueue<MyTestObject>& myContainer)
		{
			uint64_t itemCount = 0;

			std::cout << std::this_thread::get_id() << " - LoadTest_2 Worker started." << std::endl;
			while (!st.stop_requested())
			{
				auto item = myContainer.tryWaitItem();
				if (item.has_value()) itemCount++;
			}
			std::cout << std::this_thread::get_id() << " - LoadTest_2 Worker ended..." << itemCount << std::endl;
		};

		// Create the workers..
		std::array<std::jthread, THREAD_COUNT> threadPool {};
		for (int i = 0; i < THREAD_COUNT; i++)
		{
			threadPool[i] = std::jthread(workerFunction, std::ref(myContainer));
		}

		for (auto i = 0; i < ITERATION_COUNT; i++)
		{
			myContainer.emplace(MyTestObject(std::format("MyObject(2):{}:{}", i, ITERATION_COUNT)));
		}

		// Wait until we're done spooling all items..
		while (myContainer.size() > 0)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		// Immediately request stop..
		for (auto& t : threadPool)
		{
			std::cout << "LoadTest_2 - Force stopping thread #" << t.get_id() << std::endl;
			t.request_stop();
		}
	}
	catch (...)
	{
	}

	std::cout << std::format("\n{} - Expected Iteration count: {}  Adds: {}  Removes: {}  size: {}   Destroyed: {} ++--++\n",
	                         __func__,
	                         ITERATION_COUNT,
	                         myContainer.addCounter(),
	                         myContainer.removeCounter(),
	                         myContainer.size(),
	                         CountObjectsDestroyed.load());
	EXPECT_EQ(ITERATION_COUNT, myContainer.addCounter()) << myContainer.size();
	EXPECT_EQ(ITERATION_COUNT, myContainer.removeCounter()) << myContainer.size();
}

TEST(WaitableQueueTests, LoadTest_Fail)
{
	CountObjectsDestroyed                                   = 0;
	static const auto                       ITERATION_COUNT = 10;
	static const int                        THREAD_COUNT    = 4;
	siddiqsoft::WaitableQueue<MyTestObject> myContainer;

	try
	{
		// The worker function for each thread..
		auto workerFunction = [](std::stop_token st, siddiqsoft::WaitableQueue<MyTestObject>& myContainer)
		{
			uint64_t itemCount = 0;

			std::cout << std::this_thread::get_id() << " - LoadTest_Fail Worker started." << std::endl;
			while (!st.stop_requested())
			{
				auto item = myContainer.tryWaitItem();
				if (item.has_value()) itemCount++;
			}
			std::cout << std::this_thread::get_id() << " - LoadTest_Fail Worker ended..." << itemCount << std::endl;
		};

		// Create the workers..
		std::array<std::jthread, THREAD_COUNT> threadPool {};
		for (int i = 0; i < THREAD_COUNT; i++)
		{
			threadPool[i] = std::jthread(workerFunction, std::ref(myContainer));
		}

		for (auto i = 0; i < ITERATION_COUNT; i++)
		{
			myContainer.emplace(MyTestObject(std::format("MyObject(3):{}:{}", i, ITERATION_COUNT)));
			if (i > (ITERATION_COUNT / THREAD_COUNT)) threadPool[THREAD_COUNT - 1].request_stop();
			if (i > (ITERATION_COUNT / THREAD_COUNT - 1)) threadPool[THREAD_COUNT - 2].request_stop();
			if (i > (ITERATION_COUNT / THREAD_COUNT - 2)) threadPool[THREAD_COUNT - 3].request_stop();
			if (i > (ITERATION_COUNT / THREAD_COUNT - 3)) threadPool[THREAD_COUNT - 4].request_stop();
		}

		// Immediately request stop..
		// This SHOULD leave some items unprocessed in the pool
		for (auto& t : threadPool)
		{
			std::cout << "LoadTest_Fail - Force stopping thread #" << t.get_id() << std::endl;
			t.request_stop();
		}
	}
	catch (...)
	{
	}

	std::cout << std::format("\n{} - Expected Iteration count: {}  Adds: {}  Removes: {}  size: {}   Destroyed: {} ++--++\n",
	                         __func__,
	                         ITERATION_COUNT,
	                         myContainer.addCounter(),
	                         myContainer.removeCounter(),
	                         myContainer.size(),
	                         CountObjectsDestroyed.load());
	EXPECT_EQ(ITERATION_COUNT, myContainer.addCounter()) << myContainer.size();
	EXPECT_NE(ITERATION_COUNT, myContainer.removeCounter()) << myContainer.size();
}

TEST(WaitableQueueTests, LoadTest_3)
{
	CountObjectsDestroyed                                  = 0;
	static const auto                      ITERATION_COUNT = 100;
	static const int                       THREAD_COUNT    = 4;
	siddiqsoft::WaitableQueue<std::string> myContainer;

	try
	{
		// The worker function for each thread..
		auto workerFunction = [](std::stop_token st, siddiqsoft::WaitableQueue<std::string>& myContainer)
		{
			uint64_t itemCount = 0;

			std::cout << std::this_thread::get_id() << " - LoadTest_1 Worker started." << std::endl;
			while (!st.stop_requested())
			{
				auto item = myContainer.tryWaitItem();
				if (item.has_value()) itemCount++;
				// Simulate some work ..
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
			std::cout << std::this_thread::get_id() << " - LoadTest_1 Worker ended..." << itemCount << std::endl;
		};

		// Create the workers..
		std::array<std::jthread, THREAD_COUNT> threadPool {};
		for (int i = 0; i < THREAD_COUNT; i++)
		{
			threadPool[i] = std::jthread(workerFunction, std::ref(myContainer));
		}

		for (auto i = 0; i < ITERATION_COUNT; i++)
		{
			myContainer.emplace(std::format("Item---------------------------: {}", i));
		}

		EXPECT_NE(ITERATION_COUNT, myContainer.removeCounter()) << myContainer.size();
		EXPECT_EQ(ITERATION_COUNT, myContainer.addCounter()) << myContainer.size();

		myContainer.waitUntilEmpty();

		EXPECT_EQ(ITERATION_COUNT, myContainer.removeCounter()) << myContainer.size();

		// Immediately request stop..
		for (auto& t : threadPool)
		{
			std::cout << "Force stopping thread #" << t.get_id() << std::endl;
			t.request_stop();
		}
	}
	catch (...)
	{
	}

	std::cout << std::format("{} - Expected Iteration count: {}  Adds: {}  Destroyed: {}\n",
	                         __func__,
	                         ITERATION_COUNT,
	                         myContainer.addCounter(),
	                         CountObjectsDestroyed.load());
	EXPECT_EQ(ITERATION_COUNT, myContainer.addCounter()) << myContainer.size();
}


// ============================================================================
// Additional unit-level coverage tests for WaitableQueue
// ============================================================================

// --- Basic push and tryWaitItem ---

TEST(WaitableQueueTests, BasicPushAndPop)
{
	siddiqsoft::WaitableQueue<std::string> queue;

	queue.push(std::string("hello"));
	EXPECT_EQ(1u, queue.size());
	EXPECT_EQ(1u, queue.addCounter());
	EXPECT_EQ(0u, queue.removeCounter());

	auto item = queue.tryWaitItem(std::chrono::milliseconds(100));
	ASSERT_TRUE(item.has_value());
	EXPECT_EQ("hello", item.value());
	EXPECT_EQ(0u, queue.size());
	EXPECT_EQ(1u, queue.removeCounter());
}

// --- Basic emplace and tryWaitItem ---

TEST(WaitableQueueTests, BasicEmplaceAndPop)
{
	siddiqsoft::WaitableQueue<std::string> queue;

	queue.emplace(std::string("world"));
	EXPECT_EQ(1u, queue.size());
	EXPECT_EQ(1u, queue.addCounter());

	auto item = queue.tryWaitItem(std::chrono::milliseconds(100));
	ASSERT_TRUE(item.has_value());
	EXPECT_EQ("world", item.value());
	EXPECT_EQ(0u, queue.size());
	EXPECT_EQ(1u, queue.removeCounter());
}

// --- tryWaitItem timeout on empty queue ---

TEST(WaitableQueueTests, TryWaitItemTimeoutOnEmpty)
{
	siddiqsoft::WaitableQueue<int> queue;

	auto start = std::chrono::steady_clock::now();
	auto item  = queue.tryWaitItem(std::chrono::milliseconds(50));
	auto end   = std::chrono::steady_clock::now();

	EXPECT_FALSE(item.has_value());
	EXPECT_EQ(0u, queue.removeCounter());
	// Verify we actually waited approximately the timeout duration
	auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	EXPECT_GE(elapsed, 40); // allow some slack
}

// --- waitUntilEmpty on already-empty queue ---

TEST(WaitableQueueTests, WaitUntilEmptyOnAlreadyEmpty)
{
	siddiqsoft::WaitableQueue<int> queue;

	auto result = queue.waitUntilEmpty(std::chrono::milliseconds(100));
	ASSERT_TRUE(result.has_value());
	EXPECT_EQ(0u, result.value());
}

// --- size on empty queue ---

TEST(WaitableQueueTests, SizeOnEmptyQueue)
{
	siddiqsoft::WaitableQueue<std::string> queue;
	EXPECT_EQ(0u, queue.size());
}

// --- Counters start at zero ---

TEST(WaitableQueueTests, CountersStartAtZero)
{
	siddiqsoft::WaitableQueue<int> queue;
	EXPECT_EQ(0u, queue.addCounter());
	EXPECT_EQ(0u, queue.removeCounter());
}

// --- FIFO ordering ---

TEST(WaitableQueueTests, FIFOOrdering)
{
	siddiqsoft::WaitableQueue<int> queue;

	for (int i = 0; i < 10; ++i)
	{
		int val = i;
		queue.push(std::move(val));
	}

	for (int i = 0; i < 10; ++i)
	{
		auto item = queue.tryWaitItem(std::chrono::milliseconds(100));
		ASSERT_TRUE(item.has_value());
		EXPECT_EQ(i, item.value());
	}

	EXPECT_EQ(0u, queue.size());
	EXPECT_EQ(10u, queue.addCounter());
	EXPECT_EQ(10u, queue.removeCounter());
}

// --- Multiple push then drain ---

TEST(WaitableQueueTests, MultiplePushThenDrain)
{
	siddiqsoft::WaitableQueue<std::string> queue;

	const int COUNT = 100;
	for (int i = 0; i < COUNT; ++i)
	{
		queue.push(std::format("item_{}", i));
	}
	EXPECT_EQ(static_cast<size_t>(COUNT), queue.size());
	EXPECT_EQ(static_cast<uint64_t>(COUNT), queue.addCounter());

	for (int i = 0; i < COUNT; ++i)
	{
		auto item = queue.tryWaitItem(std::chrono::milliseconds(100));
		ASSERT_TRUE(item.has_value());
		EXPECT_EQ(std::format("item_{}", i), item.value());
	}

	EXPECT_EQ(0u, queue.size());
	EXPECT_EQ(static_cast<uint64_t>(COUNT), queue.removeCounter());
}

// --- Push and emplace interleaved on same queue ---

TEST(WaitableQueueTests, PushAndEmplaceInterleaved)
{
	siddiqsoft::WaitableQueue<std::string> queue;

	queue.push(std::string("push1"));
	queue.emplace(std::string("emplace1"));
	queue.push(std::string("push2"));
	queue.emplace(std::string("emplace2"));

	EXPECT_EQ(4u, queue.size());
	EXPECT_EQ(4u, queue.addCounter());

	auto i1 = queue.tryWaitItem(std::chrono::milliseconds(50));
	auto i2 = queue.tryWaitItem(std::chrono::milliseconds(50));
	auto i3 = queue.tryWaitItem(std::chrono::milliseconds(50));
	auto i4 = queue.tryWaitItem(std::chrono::milliseconds(50));

	ASSERT_TRUE(i1.has_value());
	ASSERT_TRUE(i2.has_value());
	ASSERT_TRUE(i3.has_value());
	ASSERT_TRUE(i4.has_value());

	EXPECT_EQ("push1", i1.value());
	EXPECT_EQ("emplace1", i2.value());
	EXPECT_EQ("push2", i3.value());
	EXPECT_EQ("emplace2", i4.value());

	EXPECT_EQ(0u, queue.size());
	EXPECT_EQ(4u, queue.removeCounter());
}

// --- waitUntilEmpty with active consumer ---

TEST(WaitableQueueTests, WaitUntilEmptyWithConsumer)
{
	siddiqsoft::WaitableQueue<int> queue;

	const int COUNT = 20;
	for (int i = 0; i < COUNT; ++i)
	{
		int val = i;
		queue.push(std::move(val));
	}

	// Start a consumer thread
	std::jthread consumer([&](std::stop_token st) {
		while (!st.stop_requested())
		{
			auto item = queue.tryWaitItem(std::chrono::milliseconds(10));
			if (!item.has_value() && queue.size() == 0) break;
		}
	});

	auto result = queue.waitUntilEmpty(std::chrono::milliseconds(3000));
	ASSERT_TRUE(result.has_value());
	EXPECT_EQ(0u, result.value());

	consumer.request_stop();
	consumer.join();

	EXPECT_EQ(static_cast<uint64_t>(COUNT), queue.addCounter());
	EXPECT_EQ(static_cast<uint64_t>(COUNT), queue.removeCounter());
}

// --- Custom moveable type ---

struct MoveOnlyItem
{
	int                    value;
	std::unique_ptr<int>   data;

	MoveOnlyItem(int v)
		: value(v)
		, data(std::make_unique<int>(v * 10))
	{
	}
	MoveOnlyItem(MoveOnlyItem&&)            = default;
	MoveOnlyItem& operator=(MoveOnlyItem&&) = default;
	MoveOnlyItem(const MoveOnlyItem&)       = delete;
	MoveOnlyItem& operator=(const MoveOnlyItem&) = delete;
};

TEST(WaitableQueueTests, MoveOnlyType)
{
	siddiqsoft::WaitableQueue<MoveOnlyItem> queue;

	queue.push(MoveOnlyItem {42});
	queue.emplace(MoveOnlyItem {99});

	EXPECT_EQ(2u, queue.size());

	auto item1 = queue.tryWaitItem(std::chrono::milliseconds(100));
	ASSERT_TRUE(item1.has_value());
	EXPECT_EQ(42, item1->value);
	ASSERT_TRUE(item1->data);
	EXPECT_EQ(420, *item1->data);

	auto item2 = queue.tryWaitItem(std::chrono::milliseconds(100));
	ASSERT_TRUE(item2.has_value());
	EXPECT_EQ(99, item2->value);
	ASSERT_TRUE(item2->data);
	EXPECT_EQ(990, *item2->data);

	EXPECT_EQ(0u, queue.size());
}

// --- tryWaitItem returns immediately when item is available ---

TEST(WaitableQueueTests, TryWaitItemReturnsImmediately)
{
	siddiqsoft::WaitableQueue<int> queue;

	int val = 7;
	queue.push(std::move(val));

	auto start = std::chrono::steady_clock::now();
	auto item  = queue.tryWaitItem(std::chrono::milliseconds(5000)); // long timeout
	auto end   = std::chrono::steady_clock::now();

	ASSERT_TRUE(item.has_value());
	EXPECT_EQ(7, item.value());

	// Should return almost immediately, not wait 5 seconds
	auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	EXPECT_LT(elapsed, 1000);
}

// --- waitUntilEmpty returns non-zero when timeout expires with items remaining ---

TEST(WaitableQueueTests, WaitUntilEmptyTimesOut)
{
	siddiqsoft::WaitableQueue<int> queue;

	// Push items but don't consume them
	for (int i = 0; i < 5; ++i)
	{
		int val = i;
		queue.push(std::move(val));
	}

	// Very short timeout, no consumer => should return with items still in queue
	auto result = queue.waitUntilEmpty(std::chrono::milliseconds(100));
	ASSERT_TRUE(result.has_value());
	EXPECT_GT(result.value(), 0u);
	EXPECT_EQ(5u, queue.size());
}

// --- Integer type queue ---

TEST(WaitableQueueTests, IntegerQueue)
{
	siddiqsoft::WaitableQueue<int> queue;

	queue.push(int {1});
	queue.push(int {2});
	queue.push(int {3});

	EXPECT_EQ(3u, queue.size());

	auto a = queue.tryWaitItem(std::chrono::milliseconds(50));
	auto b = queue.tryWaitItem(std::chrono::milliseconds(50));
	auto c = queue.tryWaitItem(std::chrono::milliseconds(50));

	ASSERT_TRUE(a.has_value());
	ASSERT_TRUE(b.has_value());
	ASSERT_TRUE(c.has_value());
	EXPECT_EQ(1, a.value());
	EXPECT_EQ(2, b.value());
	EXPECT_EQ(3, c.value());
}

// --- Single producer, single consumer basic flow ---

TEST(WaitableQueueTests, SingleProducerSingleConsumer)
{
	siddiqsoft::WaitableQueue<int>  queue;
	std::atomic_uint64_t           consumed {0};
	const int                      ITEM_COUNT = 50;

	std::jthread consumer([&](std::stop_token st) {
		while (!st.stop_requested())
		{
			auto item = queue.tryWaitItem(std::chrono::milliseconds(20));
			if (item.has_value()) consumed.fetch_add(1, std::memory_order_relaxed);
		}
		// Drain remaining
		while (true)
		{
			auto item = queue.tryWaitItem(std::chrono::milliseconds(10));
			if (!item.has_value()) break;
			consumed.fetch_add(1, std::memory_order_relaxed);
		}
	});

	for (int i = 0; i < ITEM_COUNT; ++i)
	{
		int val = i;
		queue.push(std::move(val));
	}

	queue.waitUntilEmpty(std::chrono::milliseconds(3000));
	consumer.request_stop();
	consumer.join();

	EXPECT_EQ(static_cast<uint64_t>(ITEM_COUNT), queue.addCounter());
	EXPECT_EQ(static_cast<uint64_t>(ITEM_COUNT), consumed.load());
	EXPECT_EQ(static_cast<uint64_t>(ITEM_COUNT), queue.removeCounter());
}
// NOLINTEND(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
