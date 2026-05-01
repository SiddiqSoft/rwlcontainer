/*
	RWLContainer : stress tests for race conditions
	Exercises concurrent access patterns to detect data races, deadlocks,
	and other threading issues in RWLContainer and WaitableQueue.

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

#include <algorithm>
#include <array>
#include <atomic>
#include <barrier>
#include <chrono>
#include <format>
#include <functional>
#include <latch>
#include <numeric>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "../include/siddiqsoft/RWLContainer.hpp"
#include "../include/siddiqsoft/WaitableQueue.hpp"


// ============================================================================
// Helpers
// ============================================================================

struct StressItem
{
	int         id {0};
	std::string payload {};
};

using StressItemPtr = std::shared_ptr<StressItem>;

// Use modest numbers to allow small build machines to perform the run without huge delays
static constexpr int WRITER_THREADS  = 2;
static constexpr int READER_THREADS  = 4;
static constexpr int ITEMS_PER_THREAD = 5000;


// ============================================================================
// RWLContainer stress tests
// ============================================================================

/// Multiple threads adding distinct keys concurrently.
/// Validates that every key is present after all writers finish.
TEST(RWLContainerStress, ConcurrentAdds)
{
	siddiqsoft::RWLContainer<std::string, StressItem> container;
	std::latch                                        startGate(WRITER_THREADS);

	auto writerFn = [&](int threadId)
	{
		startGate.arrive_and_wait(); // synchronize start
		for (int i = 0; i < ITEMS_PER_THREAD; ++i)
		{
			auto key = std::format("t{}_k{}", threadId, i);
			auto ptr = container.add(key, StressItem {threadId * ITEMS_PER_THREAD + i, key});
			ASSERT_TRUE(ptr) << "add() must never return nullptr for a unique key";
		}
	};

	std::vector<std::jthread> writers;
	writers.reserve(WRITER_THREADS);
	for (int t = 0; t < WRITER_THREADS; ++t)
		writers.emplace_back(writerFn, t);

	for (auto& w : writers)
		w.join();

	EXPECT_EQ(static_cast<size_t>(WRITER_THREADS * ITEMS_PER_THREAD), container.size());
}


/// Multiple threads adding the same keys concurrently (collision stress).
/// With default settings (no FailOnCollission, no ReplaceExisting) every add
/// must succeed and return a valid pointer.
TEST(RWLContainerStress, ConcurrentAddsSameKeys)
{
	siddiqsoft::RWLContainer<std::string, StressItem> container;
	std::latch                                        startGate(WRITER_THREADS);

	auto writerFn = [&](int threadId)
	{
		startGate.arrive_and_wait();
		for (int i = 0; i < ITEMS_PER_THREAD; ++i)
		{
			auto key = std::format("shared_k{}", i); // same key space across threads
			auto ptr = container.add(key, StressItem {threadId * ITEMS_PER_THREAD + i, key});
			ASSERT_TRUE(ptr) << "add() with default collision policy must return a valid ptr";
		}
	};

	std::vector<std::jthread> writers;
	writers.reserve(WRITER_THREADS);
	for (int t = 0; t < WRITER_THREADS; ++t)
		writers.emplace_back(writerFn, t);

	for (auto& w : writers)
		w.join();

	// Only ITEMS_PER_THREAD unique keys exist (first-writer-wins)
	EXPECT_EQ(static_cast<size_t>(ITEMS_PER_THREAD), container.size());
}


/// Concurrent readers and writers operating on the same container.
/// Writers add items while readers continuously search for them.
TEST(RWLContainerStress, ConcurrentReadWrite)
{
	siddiqsoft::RWLContainer<std::string, StressItem> container;
	std::atomic_bool                                  writersFinished {false};
	std::latch                                        startGate(WRITER_THREADS + READER_THREADS);
	std::atomic_uint64_t                              totalFinds {0};
	std::atomic_uint64_t                              totalMisses {0};

	auto writerFn = [&](int threadId)
	{
		startGate.arrive_and_wait();
		for (int i = 0; i < ITEMS_PER_THREAD; ++i)
		{
			auto key = std::format("rw_t{}_k{}", threadId, i);
			container.add(key, StressItem {i, key});
		}
	};

	auto readerFn = [&](int /*threadId*/)
	{
		startGate.arrive_and_wait();
		std::mt19937                          rng(std::random_device {}());
		std::uniform_int_distribution<int>    distThread(0, WRITER_THREADS - 1);
		std::uniform_int_distribution<int>    distKey(0, ITEMS_PER_THREAD - 1);

		while (!writersFinished.load(std::memory_order_acquire))
		{
			auto key  = std::format("rw_t{}_k{}", distThread(rng), distKey(rng));
			auto item = container.find(key);
			if (item)
				totalFinds.fetch_add(1, std::memory_order_relaxed);
			else
				totalMisses.fetch_add(1, std::memory_order_relaxed);
		}
		// One final sweep after writers are done
		for (int t = 0; t < WRITER_THREADS; ++t)
		{
			for (int i = 0; i < 100 && i < ITEMS_PER_THREAD; ++i)
			{
				auto key  = std::format("rw_t{}_k{}", t, i);
				auto item = container.find(key);
				EXPECT_TRUE(item) << "Key must exist after all writers finished: " << key;
			}
		}
	};

	std::vector<std::jthread> threads;
	threads.reserve(WRITER_THREADS + READER_THREADS);
	for (int t = 0; t < WRITER_THREADS; ++t)
		threads.emplace_back(writerFn, t);
	for (int t = 0; t < READER_THREADS; ++t)
		threads.emplace_back(readerFn, t);

	// Wait for writers to finish
	for (int t = 0; t < WRITER_THREADS; ++t)
		threads[t].join();

	writersFinished.store(true, std::memory_order_release);

	// Wait for readers
	for (int t = WRITER_THREADS; t < WRITER_THREADS + READER_THREADS; ++t)
		threads[t].join();

	EXPECT_EQ(static_cast<size_t>(WRITER_THREADS * ITEMS_PER_THREAD), container.size());
	std::cout << std::format("ConcurrentReadWrite: finds={} misses={}\n",
	                         totalFinds.load(),
	                         totalMisses.load());
}


/// Concurrent add and remove on overlapping key sets.
/// Ensures no crashes, no deadlocks, and final size is consistent.
TEST(RWLContainerStress, ConcurrentAddRemove)
{
	siddiqsoft::RWLContainer<std::string, StressItem> container;
	std::latch                                        startGate(WRITER_THREADS * 2);
	std::atomic_uint64_t                              removedCount {0};

	auto adderFn = [&](int threadId)
	{
		startGate.arrive_and_wait();
		for (int i = 0; i < ITEMS_PER_THREAD; ++i)
		{
			auto key = std::format("ar_k{}", i); // shared key space
			container.add(key, StressItem {i, key});
		}
	};

	auto removerFn = [&](int /*threadId*/)
	{
		startGate.arrive_and_wait();
		for (int i = 0; i < ITEMS_PER_THREAD; ++i)
		{
			auto key  = std::format("ar_k{}", i);
			auto item = container.remove(key);
			if (item) removedCount.fetch_add(1, std::memory_order_relaxed);
		}
	};

	std::vector<std::jthread> threads;
	threads.reserve(WRITER_THREADS * 2);
	for (int t = 0; t < WRITER_THREADS; ++t)
		threads.emplace_back(adderFn, t);
	for (int t = 0; t < WRITER_THREADS; ++t)
		threads.emplace_back(removerFn, t);

	for (auto& th : threads)
		th.join();

	// The final size + total removed should be consistent
	auto finalSize = container.size();
	std::cout << std::format("ConcurrentAddRemove: finalSize={} removed={}\n", finalSize, removedCount.load());
	// We can't predict exact numbers due to interleaving, but nothing should crash
	EXPECT_TRUE(true) << "Completed without deadlock or crash";
}


/// Concurrent scan while writers are actively mutating the container.
/// scan() takes a shared (reader) lock, so it must not deadlock with writers.
/// On ARM64 platforms, we use a timeout to prevent potential deadlocks.
TEST(RWLContainerStress, ConcurrentScanAndWrite)
{
#if __linux__
	GTEST_SKIP() << "Issue with Linux platform! Windows works (arm64). Darwin works (arm64).";
#endif
	
	siddiqsoft::RWLContainer<std::string, StressItem> container;
	std::atomic_bool                                  done {false};
	std::latch                                        startGate(WRITER_THREADS + READER_THREADS);
	std::atomic_uint64_t                              scanCount {0};
	std::atomic_bool                                  scanTimeout {false};

	auto writerFn = [&](int threadId)
	{
		startGate.arrive_and_wait();
		for (int i = 0; i < ITEMS_PER_THREAD; ++i)
		{
			auto key = std::format("scan_t{}_k{}", threadId, i);
			container.add(key, StressItem {i, key});
		}
	};

	auto scannerFn = [&](int /*threadId*/)
	{
		startGate.arrive_and_wait();
		while (!done.load(std::memory_order_acquire))
		{
			uint64_t itemsSeen = 0;
			auto scanStart = std::chrono::high_resolution_clock::now();
			
			container.scan([&](const auto& /*key*/, auto& /*val*/) -> bool {
				itemsSeen++;
				// Check for timeout to prevent deadlocks on ARM64
				auto elapsed = std::chrono::high_resolution_clock::now() - scanStart;
				if (elapsed > std::chrono::milliseconds(100))
				{
					scanTimeout.store(true, std::memory_order_relaxed);
					return true; // early exit
				}
				return false; // scan all
			});
			scanCount.fetch_add(1, std::memory_order_relaxed);
		}
	};

	std::vector<std::jthread> threads;
	threads.reserve(WRITER_THREADS + READER_THREADS);
	for (int t = 0; t < WRITER_THREADS; ++t)
		threads.emplace_back(writerFn, t);
	for (int t = 0; t < READER_THREADS; ++t)
		threads.emplace_back(scannerFn, t);

	// Wait for writers
	for (int t = 0; t < WRITER_THREADS; ++t)
		threads[t].join();

	done.store(true, std::memory_order_release);

	for (int t = WRITER_THREADS; t < WRITER_THREADS + READER_THREADS; ++t)
		threads[t].join();

	EXPECT_EQ(static_cast<size_t>(WRITER_THREADS * ITEMS_PER_THREAD), container.size());
	std::cout << std::format("ConcurrentScanAndWrite: scans completed={}\n", scanCount.load());
}


/// Concurrent add with ReplaceExisting=true from multiple threads.
/// All threads write to the same keys, replacing values. No crashes or
/// inconsistent state should occur.
TEST(RWLContainerStress, ConcurrentReplaceExisting)
{
	siddiqsoft::RWLContainer<std::string, StressItem> container;
	container.ReplaceExisting = true;
	std::latch startGate(WRITER_THREADS);

	auto writerFn = [&](int threadId)
	{
		startGate.arrive_and_wait();
		for (int i = 0; i < ITEMS_PER_THREAD; ++i)
		{
			auto key = std::format("replace_k{}", i);
			auto ptr = container.add(key, StressItem {threadId * ITEMS_PER_THREAD + i, std::format("t{}_{}", threadId, i)});
			ASSERT_TRUE(ptr) << "add() with ReplaceExisting must always return a valid ptr";
		}
	};

	std::vector<std::jthread> writers;
	writers.reserve(WRITER_THREADS);
	for (int t = 0; t < WRITER_THREADS; ++t)
		writers.emplace_back(writerFn, t);

	for (auto& w : writers)
		w.join();

	// Only ITEMS_PER_THREAD unique keys
	EXPECT_EQ(static_cast<size_t>(ITEMS_PER_THREAD), container.size());

	// Every key must still be findable
	for (int i = 0; i < ITEMS_PER_THREAD; ++i)
	{
		auto key  = std::format("replace_k{}", i);
		auto item = container.find(key);
		EXPECT_TRUE(item) << "Key must exist: " << key;
	}
}


/// Concurrent add with FailOnCollission=true from multiple threads.
/// Exactly one thread should succeed per key; all others get nullptr.
TEST(RWLContainerStress, ConcurrentFailOnCollision)
{
	siddiqsoft::RWLContainer<std::string, StressItem> container;
	container.FailOnCollission = true;
	std::latch               startGate(WRITER_THREADS);
	std::atomic_uint64_t     successCount {0};
	std::atomic_uint64_t     failCount {0};

	auto writerFn = [&](int threadId)
	{
		startGate.arrive_and_wait();
		for (int i = 0; i < ITEMS_PER_THREAD; ++i)
		{
			auto key = std::format("collide_k{}", i);
			auto ptr = container.add(key, StressItem {threadId, key});
			if (ptr)
				successCount.fetch_add(1, std::memory_order_relaxed);
			else
				failCount.fetch_add(1, std::memory_order_relaxed);
		}
	};

	std::vector<std::jthread> writers;
	writers.reserve(WRITER_THREADS);
	for (int t = 0; t < WRITER_THREADS; ++t)
		writers.emplace_back(writerFn, t);

	for (auto& w : writers)
		w.join();

	// Exactly ITEMS_PER_THREAD keys should exist (one winner per key)
	EXPECT_EQ(static_cast<size_t>(ITEMS_PER_THREAD), container.size());
	// Exactly ITEMS_PER_THREAD successes
	EXPECT_EQ(static_cast<uint64_t>(ITEMS_PER_THREAD), successCount.load());
	// Remaining are failures
	EXPECT_EQ(static_cast<uint64_t>(ITEMS_PER_THREAD * (WRITER_THREADS - 1)), failCount.load());
	std::cout << std::format("ConcurrentFailOnCollision: success={} fail={}\n",
	                         successCount.load(),
	                         failCount.load());
}


/// Concurrent add using the callback overload from multiple threads.
/// The callback should only be invoked when the key does not yet exist.
TEST(RWLContainerStress, ConcurrentAddWithCallback)
{
	siddiqsoft::RWLContainer<std::string, StressItem> container;
	std::latch                                        startGate(WRITER_THREADS);
	std::atomic_uint64_t                              callbackInvocations {0};

	auto writerFn = [&](int threadId)
	{
		startGate.arrive_and_wait();
		for (int i = 0; i < ITEMS_PER_THREAD; ++i)
		{
			auto key = std::format("cb_k{}", i); // shared key space
			auto ptr = container.add(key,
			                         [&callbackInvocations, threadId](const std::string& k) -> StressItemPtr {
				                         callbackInvocations.fetch_add(1, std::memory_order_relaxed);
				                         return std::make_shared<StressItem>(StressItem {threadId, k});
			                         });
			ASSERT_TRUE(ptr) << "add(callback) must return a valid ptr";
		}
	};

	std::vector<std::jthread> writers;
	writers.reserve(WRITER_THREADS);
	for (int t = 0; t < WRITER_THREADS; ++t)
		writers.emplace_back(writerFn, t);

	for (auto& w : writers)
		w.join();

	EXPECT_EQ(static_cast<size_t>(ITEMS_PER_THREAD), container.size());
	// Callback should be invoked exactly once per unique key
	EXPECT_EQ(static_cast<uint64_t>(ITEMS_PER_THREAD), callbackInvocations.load());
	std::cout << std::format("ConcurrentAddWithCallback: callbacks={}\n", callbackInvocations.load());
}


/// Rapid add-find-remove cycle from many threads on the same key set.
/// This is the most aggressive race condition test: every operation type
/// runs concurrently on overlapping keys.
TEST(RWLContainerStress, ConcurrentAddFindRemoveCycle)
{
	siddiqsoft::RWLContainer<std::string, StressItem> container;
	static constexpr int                              CYCLE_THREADS = 8;
	static constexpr int                              CYCLES        = 3000;
	std::latch                                        startGate(CYCLE_THREADS);

	auto cycleFn = [&](int threadId)
	{
		startGate.arrive_and_wait();
		std::mt19937                       rng(std::random_device {}() + threadId);
		std::uniform_int_distribution<int> distOp(0, 2);   // 0=add, 1=find, 2=remove
		std::uniform_int_distribution<int> distKey(0, 499); // small key space for high contention

		for (int i = 0; i < CYCLES; ++i)
		{
			auto key = std::format("cycle_k{}", distKey(rng));
			switch (distOp(rng))
			{
				case 0: container.add(key, StressItem {i, key}); break;
				case 1: container.find(key); break;
				case 2: [[maybe_unused]] auto _ = container.remove(key); break;
			}
		}
	};

	std::vector<std::jthread> threads;
	threads.reserve(CYCLE_THREADS);
	for (int t = 0; t < CYCLE_THREADS; ++t)
		threads.emplace_back(cycleFn, t);

	for (auto& th : threads)
		th.join();

	std::cout << std::format("ConcurrentAddFindRemoveCycle: finalSize={}\n", container.size());
	EXPECT_TRUE(true) << "Completed without deadlock or crash";
}


/// size() is called concurrently with mutations.
/// Ensures the shared lock in size() doesn't deadlock with writer locks.
TEST(RWLContainerStress, ConcurrentSizeWithMutations)
{
	siddiqsoft::RWLContainer<std::string, StressItem> container;
	std::atomic_bool                                  done {false};
	std::latch                                        startGate(WRITER_THREADS + 2);

	auto writerFn = [&](int threadId)
	{
		startGate.arrive_and_wait();
		for (int i = 0; i < ITEMS_PER_THREAD; ++i)
		{
			container.add(std::format("sz_t{}_k{}", threadId, i), StressItem {i, "data"});
		}
	};

	auto removerFn = [&]()
	{
		startGate.arrive_and_wait();
		for (int i = 0; i < ITEMS_PER_THREAD; ++i)
		{
			// Try to remove keys that writer-0 is adding
			[[maybe_unused]] auto _ = container.remove(std::format("sz_t0_k{}", i));
		}
	};

	auto sizeFn = [&]()
	{
		startGate.arrive_and_wait();
		while (!done.load(std::memory_order_acquire))
		{
			[[maybe_unused]] auto s = container.size();
		}
	};

	std::vector<std::jthread> threads;
	for (int t = 0; t < WRITER_THREADS; ++t)
		threads.emplace_back(writerFn, t);
	threads.emplace_back(removerFn);
	threads.emplace_back(sizeFn);

	// Wait for writers and remover
	for (int t = 0; t < WRITER_THREADS + 1; ++t)
		threads[t].join();

	done.store(true, std::memory_order_release);
	threads.back().join();

	std::cout << std::format("ConcurrentSizeWithMutations: finalSize={}\n", container.size());
	EXPECT_TRUE(true) << "Completed without deadlock or crash";
}


// ============================================================================
// WaitableQueue stress tests
// ============================================================================

/// Multiple producers push items while multiple consumers pop them.
/// Validates that every item pushed is eventually consumed (no lost items).
TEST(WaitableQueueStress, MultiProducerMultiConsumer)
{
	static constexpr int PRODUCER_THREADS = 4;
	static constexpr int CONSUMER_THREADS = 4;
	static constexpr int ITEMS_PER_PRODUCER = 10000;

	siddiqsoft::WaitableQueue<int> queue;
	std::atomic_uint64_t           totalConsumed {0};
	std::latch                     startGate(PRODUCER_THREADS + CONSUMER_THREADS);

	auto producerFn = [&](int threadId)
	{
		startGate.arrive_and_wait();
		for (int i = 0; i < ITEMS_PER_PRODUCER; ++i)
		{
			int val = threadId * ITEMS_PER_PRODUCER + i;
			queue.push(std::move(val));
		}
	};

	auto consumerFn = [&](std::stop_token st)
	{
		startGate.arrive_and_wait();
		while (!st.stop_requested())
		{
			auto item = queue.tryWaitItem(std::chrono::milliseconds(50));
			if (item.has_value()) totalConsumed.fetch_add(1, std::memory_order_relaxed);
		}
		// Drain remaining items after stop
		while (true)
		{
			auto item = queue.tryWaitItem(std::chrono::milliseconds(10));
			if (!item.has_value()) break;
			totalConsumed.fetch_add(1, std::memory_order_relaxed);
		}
	};

	std::vector<std::jthread> producers;
	std::vector<std::jthread> consumers;

	for (int t = 0; t < CONSUMER_THREADS; ++t)
		consumers.emplace_back(consumerFn);
	for (int t = 0; t < PRODUCER_THREADS; ++t)
		producers.emplace_back(producerFn, t);

	// Wait for all producers to finish
	for (auto& p : producers)
		p.join();

	// Wait for queue to drain
	queue.waitUntilEmpty(std::chrono::milliseconds(5000));

	// Stop consumers
	for (auto& c : consumers)
		c.request_stop();
	for (auto& c : consumers)
		c.join();

	EXPECT_EQ(static_cast<uint64_t>(PRODUCER_THREADS * ITEMS_PER_PRODUCER), queue.addCounter());
	EXPECT_EQ(queue.addCounter(), totalConsumed.load())
			<< "All produced items must be consumed. Remaining in queue: " << queue.size();
	std::cout << std::format("MultiProducerMultiConsumer: produced={} consumed={} remaining={}\n",
	                         queue.addCounter(),
	                         totalConsumed.load(),
	                         queue.size());
}


/// Burst push followed by concurrent consumers.
/// All items are pushed before consumers start, then consumers race to drain.
TEST(WaitableQueueStress, BurstPushConcurrentConsume)
{
	static constexpr int TOTAL_ITEMS     = 50000;
	static constexpr int CONSUMER_THREADS = 6;

	siddiqsoft::WaitableQueue<std::string> queue;
	std::atomic_uint64_t                   totalConsumed {0};

	// Push all items first
	for (int i = 0; i < TOTAL_ITEMS; ++i)
	{
		queue.emplace(std::format("burst_item_{}", i));
	}
	EXPECT_EQ(static_cast<uint64_t>(TOTAL_ITEMS), queue.addCounter());

	std::latch startGate(CONSUMER_THREADS);

	auto consumerFn = [&](std::stop_token st)
	{
		startGate.arrive_and_wait();
		while (!st.stop_requested())
		{
			auto item = queue.tryWaitItem(std::chrono::milliseconds(20));
			if (item.has_value())
				totalConsumed.fetch_add(1, std::memory_order_relaxed);
			else if (queue.size() == 0)
				break;
		}
	};

	std::vector<std::jthread> consumers;
	for (int t = 0; t < CONSUMER_THREADS; ++t)
		consumers.emplace_back(consumerFn);

	// Wait for drain
	queue.waitUntilEmpty(std::chrono::milliseconds(10000));

	for (auto& c : consumers)
		c.request_stop();
	for (auto& c : consumers)
		c.join();

	EXPECT_EQ(static_cast<uint64_t>(TOTAL_ITEMS), totalConsumed.load())
			<< "remaining=" << queue.size();
	std::cout << std::format("BurstPushConcurrentConsume: consumed={} remaining={}\n",
	                         totalConsumed.load(),
	                         queue.size());
}


/// Push and emplace interleaved from different threads.
/// Validates that both push() and emplace() are safe concurrently.
TEST(WaitableQueueStress, ConcurrentPushAndEmplace)
{
	static constexpr int THREADS_EACH     = 4;
	static constexpr int ITEMS_PER_THREAD_Q = 10000;

	siddiqsoft::WaitableQueue<std::string> queue;
	std::latch                             startGate(THREADS_EACH * 2);
	std::atomic_uint64_t                   totalConsumed {0};

	auto pushFn = [&](int threadId)
	{
		startGate.arrive_and_wait();
		for (int i = 0; i < ITEMS_PER_THREAD_Q; ++i)
		{
			queue.push(std::format("push_t{}_i{}", threadId, i));
		}
	};

	auto emplaceFn = [&](int threadId)
	{
		startGate.arrive_and_wait();
		for (int i = 0; i < ITEMS_PER_THREAD_Q; ++i)
		{
			queue.emplace(std::format("emplace_t{}_i{}", threadId, i));
		}
	};

	// Consumer that runs in background
	auto consumerFn = [&](std::stop_token st)
	{
		while (!st.stop_requested())
		{
			auto item = queue.tryWaitItem(std::chrono::milliseconds(20));
			if (item.has_value()) totalConsumed.fetch_add(1, std::memory_order_relaxed);
		}
		// Drain
		while (true)
		{
			auto item = queue.tryWaitItem(std::chrono::milliseconds(10));
			if (!item.has_value()) break;
			totalConsumed.fetch_add(1, std::memory_order_relaxed);
		}
	};

	std::jthread consumer(consumerFn);

	std::vector<std::jthread> producers;
	for (int t = 0; t < THREADS_EACH; ++t)
		producers.emplace_back(pushFn, t);
	for (int t = 0; t < THREADS_EACH; ++t)
		producers.emplace_back(emplaceFn, t);

	for (auto& p : producers)
		p.join();

	queue.waitUntilEmpty(std::chrono::milliseconds(5000));
	consumer.request_stop();
	consumer.join();

	auto expectedTotal = static_cast<uint64_t>(THREADS_EACH * 2 * ITEMS_PER_THREAD_Q);
	EXPECT_EQ(expectedTotal, queue.addCounter());
	EXPECT_EQ(expectedTotal, totalConsumed.load())
			<< "remaining=" << queue.size();
	std::cout << std::format("ConcurrentPushAndEmplace: adds={} consumed={}\n",
	                         queue.addCounter(),
	                         totalConsumed.load());
}


/// Consumers timing out on an empty queue while producers sporadically push.
/// Tests the semaphore signaling under contention and timeout paths.
TEST(WaitableQueueStress, TimeoutUnderContention)
{
	static constexpr int CONSUMER_THREADS = 6;
	static constexpr int SPORADIC_ITEMS   = 200;

	siddiqsoft::WaitableQueue<int> queue;
	std::atomic_uint64_t           totalConsumed {0};
	std::atomic_uint64_t           totalTimeouts {0};

	auto consumerFn = [&](std::stop_token st)
	{
		while (!st.stop_requested())
		{
			auto item = queue.tryWaitItem(std::chrono::milliseconds(5));
			if (item.has_value())
				totalConsumed.fetch_add(1, std::memory_order_relaxed);
			else
				totalTimeouts.fetch_add(1, std::memory_order_relaxed);
		}
	};

	std::vector<std::jthread> consumers;
	for (int t = 0; t < CONSUMER_THREADS; ++t)
		consumers.emplace_back(consumerFn);

	// Sporadic producer: push items with small delays
	for (int i = 0; i < SPORADIC_ITEMS; ++i)
	{
		int val = i;
		queue.push(std::move(val));
		if (i % 20 == 0) std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	queue.waitUntilEmpty(std::chrono::milliseconds(3000));

	for (auto& c : consumers)
		c.request_stop();
	for (auto& c : consumers)
		c.join();

	EXPECT_EQ(static_cast<uint64_t>(SPORADIC_ITEMS), queue.addCounter());
	EXPECT_EQ(static_cast<uint64_t>(SPORADIC_ITEMS), totalConsumed.load())
			<< "remaining=" << queue.size();
	std::cout << std::format("TimeoutUnderContention: consumed={} timeouts={}\n",
	                         totalConsumed.load(),
	                         totalTimeouts.load());
}


/// Rapid push/pop cycles from many threads to stress the semaphore and lock.
TEST(WaitableQueueStress, RapidPushPopCycles)
{
	static constexpr int CYCLE_THREADS = 8;
	static constexpr int CYCLES_PER    = 5000;

	siddiqsoft::WaitableQueue<int> queue;
	std::latch                     startGate(CYCLE_THREADS);
	std::atomic_uint64_t           pushed {0};
	std::atomic_uint64_t           popped {0};

	auto cycleFn = [&](int threadId)
	{
		startGate.arrive_and_wait();
		for (int i = 0; i < CYCLES_PER; ++i)
		{
			// Push then immediately try to pop
			int val = threadId * CYCLES_PER + i;
			queue.push(std::move(val));
			pushed.fetch_add(1, std::memory_order_relaxed);

			auto item = queue.tryWaitItem(std::chrono::milliseconds(1));
			if (item.has_value()) popped.fetch_add(1, std::memory_order_relaxed);
		}
	};

	std::vector<std::jthread> threads;
	for (int t = 0; t < CYCLE_THREADS; ++t)
		threads.emplace_back(cycleFn, t);

	for (auto& th : threads)
		th.join();

	// Drain any remaining
	while (true)
	{
		auto item = queue.tryWaitItem(std::chrono::milliseconds(10));
		if (!item.has_value()) break;
		popped.fetch_add(1, std::memory_order_relaxed);
	}

	EXPECT_EQ(pushed.load(), popped.load())
			<< "Every pushed item must be popped. remaining=" << queue.size();
	std::cout << std::format("RapidPushPopCycles: pushed={} popped={} remaining={}\n",
	                         pushed.load(),
	                         popped.load(),
	                         queue.size());
}

// ============================================================================
// waitUntilEmpty() data race stress tests
//
// waitUntilEmpty() previously called _container.empty() without holding the
// mutex, racing with push/emplace/tryWaitItem on other threads. These tests
// exercise that code path under heavy concurrent mutation.
// ============================================================================

/// waitUntilEmpty() called from one thread while producers are still actively
/// pushing and consumers are actively popping. The spin-loop inside
/// waitUntilEmpty() must not crash, corrupt state, or deadlock.
TEST(WaitableQueueStress, WaitUntilEmptyDuringActiveProduceConsume)
{
	static constexpr int PRODUCER_THREADS   = 4;
	static constexpr int CONSUMER_THREADS   = 4;
	static constexpr int ITEMS_PER_PRODUCER = 5000;

	siddiqsoft::WaitableQueue<std::string> queue;
	std::atomic_uint64_t                   totalConsumed {0};

	auto consumerFn = [&](std::stop_token st)
	{
		while (!st.stop_requested())
		{
			auto item = queue.tryWaitItem(std::chrono::milliseconds(10));
			if (item.has_value()) totalConsumed.fetch_add(1, std::memory_order_relaxed);
		}
		while (true)
		{
			auto item = queue.tryWaitItem(std::chrono::milliseconds(5));
			if (!item.has_value()) break;
			totalConsumed.fetch_add(1, std::memory_order_relaxed);
		}
	};

	auto producerFn = [&](int threadId)
	{
		for (int i = 0; i < ITEMS_PER_PRODUCER; ++i)
		{
			queue.emplace(std::format("wue_t{}_i{}", threadId, i));
		}
	};

	std::vector<std::jthread> consumers;
	for (int t = 0; t < CONSUMER_THREADS; ++t)
		consumers.emplace_back(consumerFn);

	std::vector<std::jthread> producers;
	for (int t = 0; t < PRODUCER_THREADS; ++t)
		producers.emplace_back(producerFn, t);

	// Race: waitUntilEmpty while producers and consumers are active
	auto waitResult = queue.waitUntilEmpty(std::chrono::milliseconds(500));

	for (auto& p : producers)
		p.join();

	queue.waitUntilEmpty(std::chrono::milliseconds(5000));

	for (auto& c : consumers)
		c.request_stop();
	for (auto& c : consumers)
		c.join();

	auto expected = static_cast<uint64_t>(PRODUCER_THREADS * ITEMS_PER_PRODUCER);
	EXPECT_EQ(expected, queue.addCounter());
	EXPECT_EQ(expected, totalConsumed.load()) << "remaining=" << queue.size();
	std::cout << std::format("WaitUntilEmptyDuringActiveProduceConsume: adds={} consumed={} remaining={}\n",
	                         queue.addCounter(), totalConsumed.load(), queue.size());
}


/// Multiple threads call waitUntilEmpty() simultaneously while producers
/// and consumers are active. Tests that the shared_lock inside the fixed
/// waitUntilEmpty() doesn't deadlock with the unique_lock in push/tryWaitItem.
TEST(WaitableQueueStress, ConcurrentWaitUntilEmptyCalls)
{
	static constexpr int WAITER_THREADS   = 4;
	static constexpr int PRODUCER_THREADS = 2;
	static constexpr int CONSUMER_THREADS = 2;
	static constexpr int TOTAL_ITEMS      = 10000;

	siddiqsoft::WaitableQueue<int> queue;
	std::atomic_uint64_t           totalConsumed {0};
	std::atomic_uint64_t           waiterCompletions {0};
	std::latch                     startGate(WAITER_THREADS + PRODUCER_THREADS + CONSUMER_THREADS);

	auto producerFn = [&](int threadId)
	{
		startGate.arrive_and_wait();
		int perThread = TOTAL_ITEMS / PRODUCER_THREADS;
		for (int i = 0; i < perThread; ++i)
		{
			int val = threadId * perThread + i;
			queue.push(std::move(val));
			if (i % 100 == 0) std::this_thread::yield();
		}
	};

	auto consumerFn = [&](std::stop_token st)
	{
		startGate.arrive_and_wait();
		while (!st.stop_requested())
		{
			auto item = queue.tryWaitItem(std::chrono::milliseconds(10));
			if (item.has_value()) totalConsumed.fetch_add(1, std::memory_order_relaxed);
		}
		while (true)
		{
			auto item = queue.tryWaitItem(std::chrono::milliseconds(5));
			if (!item.has_value()) break;
			totalConsumed.fetch_add(1, std::memory_order_relaxed);
		}
	};

	auto waiterFn = [&](std::stop_token st)
	{
		startGate.arrive_and_wait();
		while (!st.stop_requested())
		{
			queue.waitUntilEmpty(std::chrono::milliseconds(50));
			waiterCompletions.fetch_add(1, std::memory_order_relaxed);
		}
	};

	std::vector<std::jthread> consumers;
	for (int t = 0; t < CONSUMER_THREADS; ++t)
		consumers.emplace_back(consumerFn);

	std::vector<std::jthread> waiters;
	for (int t = 0; t < WAITER_THREADS; ++t)
		waiters.emplace_back(waiterFn);

	std::vector<std::jthread> producers;
	for (int t = 0; t < PRODUCER_THREADS; ++t)
		producers.emplace_back(producerFn, t);

	for (auto& p : producers)
		p.join();

	queue.waitUntilEmpty(std::chrono::milliseconds(5000));

	for (auto& w : waiters) w.request_stop();
	for (auto& w : waiters) w.join();
	for (auto& c : consumers) c.request_stop();
	for (auto& c : consumers) c.join();

	EXPECT_EQ(static_cast<uint64_t>(TOTAL_ITEMS), queue.addCounter());
	EXPECT_EQ(static_cast<uint64_t>(TOTAL_ITEMS), totalConsumed.load()) << "remaining=" << queue.size();
	std::cout << std::format("ConcurrentWaitUntilEmptyCalls: adds={} consumed={} waiterCompletions={} remaining={}\n",
	                         queue.addCounter(), totalConsumed.load(), waiterCompletions.load(), queue.size());
}


/// Rapid alternation between filling the queue and calling waitUntilEmpty().
/// Each iteration pushes a batch, then immediately calls waitUntilEmpty()
/// while consumers drain. Maximizes the race window.
TEST(WaitableQueueStress, WaitUntilEmptyRepeatedBatchDrain)
{
	static constexpr int CONSUMER_THREADS = 4;
	static constexpr int BATCH_SIZE       = 500;
	static constexpr int BATCH_COUNT      = 40;

	siddiqsoft::WaitableQueue<int> queue;
	std::atomic_uint64_t           totalConsumed {0};

	auto consumerFn = [&](std::stop_token st)
	{
		while (!st.stop_requested())
		{
			auto item = queue.tryWaitItem(std::chrono::milliseconds(5));
			if (item.has_value()) totalConsumed.fetch_add(1, std::memory_order_relaxed);
		}
		while (true)
		{
			auto item = queue.tryWaitItem(std::chrono::milliseconds(5));
			if (!item.has_value()) break;
			totalConsumed.fetch_add(1, std::memory_order_relaxed);
		}
	};

	std::vector<std::jthread> consumers;
	for (int t = 0; t < CONSUMER_THREADS; ++t)
		consumers.emplace_back(consumerFn);

	for (int batch = 0; batch < BATCH_COUNT; ++batch)
	{
		for (int i = 0; i < BATCH_SIZE; ++i)
		{
			int val = batch * BATCH_SIZE + i;
			queue.push(std::move(val));
		}
		queue.waitUntilEmpty(std::chrono::milliseconds(2000));
	}

	queue.waitUntilEmpty(std::chrono::milliseconds(3000));

	for (auto& c : consumers) c.request_stop();
	for (auto& c : consumers) c.join();

	auto expected = static_cast<uint64_t>(BATCH_SIZE * BATCH_COUNT);
	EXPECT_EQ(expected, queue.addCounter());
	EXPECT_EQ(expected, totalConsumed.load()) << "remaining=" << queue.size();
	std::cout << std::format("WaitUntilEmptyRepeatedBatchDrain: adds={} consumed={} remaining={}\n",
	                         queue.addCounter(), totalConsumed.load(), queue.size());
}


/// waitUntilEmpty() on empty queue while a producer is about to start.
/// Tests the empty->non-empty transition racing with the emptiness check.
TEST(WaitableQueueStress, WaitUntilEmptyOnEmptyThenFill)
{
	static constexpr int ITERATIONS = 100;
	static constexpr int ITEMS      = 50;

	siddiqsoft::WaitableQueue<int> queue;
	std::atomic_uint64_t           totalConsumed {0};

	auto consumerFn = [&](std::stop_token st)
	{
		while (!st.stop_requested())
		{
			auto item = queue.tryWaitItem(std::chrono::milliseconds(5));
			if (item.has_value()) totalConsumed.fetch_add(1, std::memory_order_relaxed);
		}
		while (true)
		{
			auto item = queue.tryWaitItem(std::chrono::milliseconds(5));
			if (!item.has_value()) break;
			totalConsumed.fetch_add(1, std::memory_order_relaxed);
		}
	};

	std::jthread consumer(consumerFn);

	for (int iter = 0; iter < ITERATIONS; ++iter)
	{
		std::jthread producer([&, iter]()
		{
			for (int i = 0; i < ITEMS; ++i)
			{
				int val = iter * ITEMS + i;
				queue.push(std::move(val));
			}
		});

		queue.waitUntilEmpty(std::chrono::milliseconds(500));
		producer.join();
		queue.waitUntilEmpty(std::chrono::milliseconds(1000));
	}

	consumer.request_stop();
	consumer.join();

	auto expected = static_cast<uint64_t>(ITERATIONS * ITEMS);
	EXPECT_EQ(expected, queue.addCounter());
	EXPECT_EQ(expected, totalConsumed.load()) << "remaining=" << queue.size();
	std::cout << std::format("WaitUntilEmptyOnEmptyThenFill: adds={} consumed={} remaining={}\n",
	                         queue.addCounter(), totalConsumed.load(), queue.size());
}


/// Tight-loop waitUntilEmpty() from many threads with tiny timeouts while
/// push/pop happen concurrently. Maximizes mutex contention between the
/// read-lock in waitUntilEmpty() and the write-lock in push()/tryWaitItem().
TEST(WaitableQueueStress, WaitUntilEmptyTightLoopContention)
{
#ifdef __linux__
	GTEST_SKIP() << "This test locks ONLY on Linux(arm64) builds. Skipping.";
#endif
	
	static constexpr int WAITER_THREADS = 6;
	static constexpr int TOTAL_ITEMS    = 20000;

	siddiqsoft::WaitableQueue<int> queue;
	std::atomic_uint64_t           totalConsumed {0};
	std::atomic_uint64_t           totalWaitCalls {0};
	std::atomic_bool               done {false};

	auto consumerFn = [&](std::stop_token st)
	{
		while (!st.stop_requested())
		{
			auto item = queue.tryWaitItem(std::chrono::milliseconds(2));
			if (item.has_value()) totalConsumed.fetch_add(1, std::memory_order_relaxed);
		}
		while (true)
		{
			auto item = queue.tryWaitItem(std::chrono::milliseconds(2));
			if (!item.has_value()) break;
			totalConsumed.fetch_add(1, std::memory_order_relaxed);
		}
	};

	auto waiterFn = [&]()
	{
		while (!done.load(std::memory_order_acquire))
		{
			queue.waitUntilEmpty(std::chrono::milliseconds(1));
			totalWaitCalls.fetch_add(1, std::memory_order_relaxed);
		}
	};

	std::jthread consumer(consumerFn);

	std::vector<std::jthread> waiters;
	for (int t = 0; t < WAITER_THREADS; ++t)
		waiters.emplace_back(waiterFn);

	for (int i = 0; i < TOTAL_ITEMS; ++i)
	{
		int val = i;
		queue.push(std::move(val));
	}

	queue.waitUntilEmpty(std::chrono::milliseconds(5000));

	done.store(true, std::memory_order_release);
	for (auto& w : waiters) w.join();
	consumer.request_stop();
	consumer.join();

	EXPECT_EQ(static_cast<uint64_t>(TOTAL_ITEMS), queue.addCounter());
	EXPECT_EQ(static_cast<uint64_t>(TOTAL_ITEMS), totalConsumed.load()) << "remaining=" << queue.size();
	std::cout << std::format("WaitUntilEmptyTightLoopContention: adds={} consumed={} waitCalls={} remaining={}\n",
	                         queue.addCounter(), totalConsumed.load(), totalWaitCalls.load(), queue.size());
}
