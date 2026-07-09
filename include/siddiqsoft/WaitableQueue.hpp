/*
	Reader-Writer lock protected queue

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

#pragma once
#include <cstddef>
#include <thread>
#ifndef WaitableQueue_HPP
#define WaitableQueue_HPP

#include <optional>
#include <chrono>
#include <mutex>
#include <iostream>
#include <concepts>
#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <atomic>
#include <shared_mutex>
#include <semaphore>
#include <type_traits>

#include "siddiqsoft/RunOnEnd.hpp"


namespace siddiqsoft
{
	template <typename T>
	concept Movable = std::is_move_constructible_v<T> && std::is_move_assignable_v<T>;


	/// @brief Thread-safe queue with timeout-based waiting for producer-consumer patterns
	///
	/// A high-performance queue designed for multi-threaded producer-consumer scenarios.
	/// Features include:
	/// - Reader-writer locking for efficient concurrent access
	/// - Semaphore-based signaling for efficient waiting
	/// - Timeout-based item retrieval
	/// - FIFO (First-In-First-Out) ordering guarantee
	/// - Non-copyable and non-movable (contains synchronization primitives)
	///
	/// @tparam StorageType Any moveable object (must satisfy Movable concept)
	///
	/// @note Non-copyable: Cannot be copied or moved due to shared_mutex and counting_semaphore
	/// @note Thread-safe: All operations are protected by std::shared_mutex
	/// @note FIFO ordering: Items are retrieved in the order they were added
	/// @note Efficient waiting: Uses std::counting_semaphore for efficient producer-consumer signaling
	///
	/// @example
	/// @code
	/// siddiqsoft::WaitableQueue<std::string> queue;
	///
	/// // Producer thread
	/// std::thread producer([&]() {
	///     for (int i = 0; i < 10; ++i) {
	///         queue.push(std::string("item_") + std::to_string(i));
	///     }
	/// });
	///
	/// // Consumer thread
	/// std::thread consumer([&]() {
	///     while (true) {
	///         auto item = queue.tryWaitItem(std::chrono::milliseconds(500));
	///         if (item) {
	///             std::cout << "Processed: " << *item << std::endl;
	///         } else {
	///             break;  // Timeout, no more items
	///         }
	///     }
	/// });
	///
	/// producer.join();
	/// consumer.join();
	/// @endcode
	template <class StorageType>
		requires Movable<StorageType>
	class WaitableQueue
	{
		using RWLock = std::unique_lock<std::shared_mutex>;
		using RLock  = std::shared_lock<std::shared_mutex>;

	public:
		/// @brief Disallow the copy assignment operator
		/// @note Queue cannot be copied due to synchronization primitives
		WaitableQueue& operator=(const WaitableQueue&) = delete;

		/// @brief Delete the copy constructor
		/// @note Queue cannot be copied due to synchronization primitives
		WaitableQueue(const WaitableQueue&) = delete;

		/// @brief Disallow move constructor
		/// @note Queue cannot be moved due to synchronization primitives
		WaitableQueue(WaitableQueue&&) = delete;

		/// @brief Disallow move assignment operator
		/// @note Queue cannot be moved due to synchronization primitives
		auto operator=(WaitableQueue&&) = delete;

		/// @brief Default constructor
		/// @note Initializes semaphore with 0 signals and empty container
		WaitableQueue() = default;

		/// @brief Default destructor
		/// @note Cleans up all resources including synchronization primitives
		~WaitableQueue() = default;

		/// @brief Adds an item to the end of the queue and signals waiting consumers
		///
		/// Moves the item into the queue and signals one waiting consumer thread.
		/// This is the primary method for producers to add items.
		///
		/// @param value The item to add (moved into queue)
		/// @return void
		///
		/// @note Thread-safe: Uses exclusive write lock
		/// @note FIFO ordering: Item is added to the end of the queue
		/// @note Signals one waiting consumer via semaphore
		/// @note Increments the add counter
		/// @note The input value is moved; original reference becomes invalid
		///
		/// @example
		/// @code
		/// queue.push(std::string("hello"));
		/// queue.push(std::move(myString));
		/// @endcode
		void push(StorageType&& value)
		{
			if (RWLock _ {_containerMutex}; true)
			{
				_counterAdds++;
				_container.push_back(std::move(value));
			}
			// Must be outside the lock!
			_signal.release();
		}

		/// @brief Constructs and adds an item to the end of the queue in-place
		///
		/// Constructs the item directly in the queue using emplace semantics.
		/// More efficient than push() when you want to construct the item in-place.
		///
		/// @param value The item to construct and add (moved into queue)
		/// @return void
		///
		/// @note Thread-safe: Uses exclusive write lock
		/// @note FIFO ordering: Item is added to the end of the queue
		/// @note Signals one waiting consumer via semaphore
		/// @note Increments the add counter
		/// @note More efficient than push() for complex types
		///
		/// @example
		/// @code
		/// queue.emplace(std::string("hello"));
		/// queue.emplace(MyType{42, "value"});
		/// @endcode
		void emplace(StorageType&& value)
		{
			if (RWLock _ {_containerMutex}; true)
			{
				_counterAdds++;
				_container.emplace_back(std::move(value));
			}
			// Must be outside the lock!
			_signal.release();
		}

		/// @brief Waits until the queue is empty or timeout occurs
		///
		/// Spins (with increasing sleep intervals) until the queue is empty or the timeout expires.
		/// Useful for graceful shutdown scenarios where you want to ensure all items are processed.
		///
		/// @param timeoutDuration Maximum time to wait (defaults to 1500ms)
		/// @return std::optional<size_t> The final queue size (0 if empty, >0 if timeout)
		///
		/// @note Thread-safe: Uses shared read lock for size checks
		/// @note Blocking: Sleeps in a loop with increasing intervals (32ms, 64ms, 96ms, ...)
		/// @note Returns immediately if queue is already empty
		/// @note Useful for shutdown: wait for queue to drain before destroying
		///
		/// @example
		/// @code
		/// // Signal producers to stop
		/// // Wait for queue to drain
		/// auto finalSize = queue.waitUntilEmpty(std::chrono::milliseconds(2000));
		/// if (finalSize && *finalSize == 0) {
		///     std::cout << "Queue is empty" << std::endl;
		/// } else {
		///     std::cout << "Timeout with " << *finalSize << " items remaining" << std::endl;
		/// }
		/// @endcode
		auto waitUntilEmpty(std::chrono::milliseconds timeoutDuration = std::chrono::milliseconds(1500)) -> bool
		{
			constexpr std::chrono::milliseconds spinInterval {32};
			std::chrono::milliseconds           spinDuration {32};

			auto isEmpty = [&]() -> bool
			{
				RLock _ {_containerMutex};
				return _container.empty();
			};

			while (!isEmpty() && (spinDuration < timeoutDuration))
			{
				// Something is in the queue.. let's spinwait
				std::this_thread::sleep_for(spinDuration);
				spinDuration += spinInterval;
			}

			return isEmpty();
		}

		/// @brief Waits for an item with timeout and returns it if available
		///
		/// Waits for the specified timeout for an item to become available in the queue.
		/// Returns immediately if an item is available, or returns empty optional if timeout occurs.
		/// This is the primary method for consumers to retrieve items.
		///
		/// @param timeoutDuration Maximum time to wait (defaults to 100ms)
		/// @return std::optional<StorageType> The dequeued item, or empty if timeout
		///
		/// @note Thread-safe: Uses exclusive write lock for item removal
		/// @note FIFO ordering: Returns items in the order they were added
		/// @note Efficient waiting: Uses std::counting_semaphore for efficient signaling
		/// @note Increments the remove counter on successful retrieval
		/// @note [[nodiscard]] attribute encourages checking the return value
		/// @note Multiple consumers can wait concurrently; only one gets each item
		///
		/// @example
		/// @code
		/// while (true) {
		///     auto item = queue.tryWaitItem(std::chrono::milliseconds(500));
		///     if (item) {
		///         std::cout << "Got: " << *item << std::endl;
		///     } else {
		///         std::cout << "Timeout" << std::endl;
		///         break;
		///     }
		/// }
		/// @endcode
		[[nodiscard]] auto tryWaitItem(std::chrono::milliseconds timeoutDuration = std::chrono::milliseconds(100))
				-> std::optional<StorageType>
		{
			// This will wait for the requested interval for an item to be
			// ready for consumption. If available, we lock and attempt to
			// pull an item.
			// It is possible to be signalled and have the item potentially
			// consumed by another thread (if you have multiple threads against
			// this object.)
			if (_signal.try_acquire_for(timeoutDuration))
			{
				if (RWLock _ {_containerMutex}; !_container.empty())
				{
					// Using this scope guard allows us to minimize object movement
					siddiqsoft::RunOnEnd scopeCleanup {[&]()
					                                   {
														   _container.pop_front();
														   _counterRemoves++;
													   }};
					// Return the object via move since it will be popped by the scope guard
					return std::make_optional(std::move(_container.front()));
				}
			}

			// empty
			return {};
		}

		/// @brief Returns the current number of items in the queue
		///
		/// Gets the current size of the queue. This is a read-only operation.
		///
		/// @return size_t The number of items currently in the queue
		///
		/// @note Thread-safe: Uses shared read lock
		/// @note O(1) operation
		/// @note Multiple threads can call size() simultaneously
		/// @note Size can change immediately after return due to concurrent operations
		///
		/// @example
		/// @code
		/// std::cout << "Queue has " << queue.size() << " items" << std::endl;
		/// @endcode
		auto size() const -> size_t const
		{
			RLock _ {_containerMutex};

			return _container.size();
		}

		/// @brief Returns the total number of items added to the queue
		///
		/// Gets the cumulative count of all items that have been added via push() or emplace().
		/// This counter never decreases and provides a measure of total throughput.
		///
		/// @return uint64_t The total number of add operations
		///
		/// @note Thread-safe: Uses atomic operation
		/// @note Cumulative: Never decreases, only increases
		/// @note Useful for diagnostics and performance monitoring
		///
		/// @example
		/// @code
		/// std::cout << "Total added: " << queue.addCounter() << std::endl;
		/// @endcode
		auto addCounter() const -> uint64_t const { return _counterAdds; }

		/// @brief Returns the total number of items successfully retrieved from the queue
		///
		/// Gets the cumulative count of all items that have been successfully retrieved via tryWaitItem().
		/// This counter never decreases and provides a measure of consumption rate.
		///
		/// @return uint64_t The total number of successful remove operations
		///
		/// @note Thread-safe: Uses atomic operation
		/// @note Cumulative: Never decreases, only increases
		/// @note Useful for diagnostics and performance monitoring
		/// @note Should eventually equal addCounter() when queue is fully drained
		///
		/// @example
		/// @code
		/// std::cout << "Total removed: " << queue.removeCounter() << std::endl;
		/// @endcode
		auto removeCounter() const -> uint64_t const { return _counterRemoves; }

#ifdef INCLUDE_NLOHMANN_JSON_HPP_
	public:
		/// @brief Serializes queue metadata to JSON
		///
		/// Returns a JSON object containing queue statistics.
		/// Only available if nlohmann/json.hpp is included before this header.
		///
		/// @return nlohmann::json JSON object with:
		///   - _typver: Type and version string
		///   - adds: Total number of add operations
		///   - removes: Total number of remove operations
		///   - size: Current number of items
		///
		/// @note Thread-safe: Uses atomic operations and shared read lock
		/// @note Requires INCLUDE_NLOHMANN_JSON_HPP_ to be defined
		///
		/// @example
		/// @code
		/// #include "nlohmann/json.hpp"
		/// auto json = queue.toJson();
		/// std::cout << json.dump(2) << std::endl;
		/// @endcode
		nlohmann::json toJson() const
		{
			return nlohmann::json {
					{"_typver", "WaitableQueue/1.5.3"}, {"adds", addCounter()}, {"removes", removeCounter()}, {"size", size()}};
		}
#endif

	private:
		/// @brief Semaphore for efficient producer-consumer signaling
		/// @note Initialized with 0 signals; incremented by push/emplace, decremented by tryWaitItem
		std::counting_semaphore<> _signal {0};

		/// @brief The underlying container (deque for efficient front/back operations)
		/// @note Stores items in FIFO order
		std::deque<StorageType> _container {};

		/// @brief Shared mutex for reader-writer locking
		/// @note Mutable to allow const methods to acquire read locks
		mutable std::shared_mutex _containerMutex;

		/// @brief Atomic counter for total add operations
		/// @note Incremented by push/emplace, never decreases
		std::atomic_uint64_t _counterAdds {0};

		/// @brief Atomic counter for total remove operations
		/// @note Incremented by successful tryWaitItem, never decreases
		std::atomic_uint64_t _counterRemoves {0};
	};
} // namespace siddiqsoft

#endif // !WaitableQueue_HPP
