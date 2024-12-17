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
#include <queue>
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


	/**
	 * @brief WaitableQueue. Object cannot be re-assigned, copied or moved as it stores a shared_mutex and counting_semaphore.
     *        Use this container in a multi-threaded scenario with workers processing IO from this queued list.
     *        Implementes a reader-writer lock to alleviate client burden.
     *        The client threads must deal with timeouts on empty queue.
	 * 
	 * @tparam StorageType Any moveable object
	 * @tparam StorageContainer Defaults to a std::queue<StorageType>
	 */
	template <class StorageType, class StorageContainer = std::queue<StorageType>>
		requires Movable<StorageType>
	class WaitableQueue
	{
		using RWLock = std::unique_lock<std::shared_mutex>;
		using RLock  = std::shared_lock<std::shared_mutex>;

	public:
		/// @brief Disallow the copy assignment operator
		WaitableQueue& operator=(const WaitableQueue&) = delete;
		/// @brief Delete the copy constructor
		WaitableQueue(const WaitableQueue&) = delete;
		/// @brief Disallow move constructor
		WaitableQueue(WaitableQueue&&) = delete;
		/// @brief Disallow move assignment operator
		auto operator=(WaitableQueue&&) = delete;
		/// @brief Default constructor.
		/// We must declare this as default since we're removing
		/// the move and copy constructors.
		WaitableQueue() = default;
		/// @brief Default destructor.
		/// We must ask for the default destructor
		~WaitableQueue() = default;

		/**
		 * @brief Push item at the end of the internal queue and signals waiting clients.
		 * 
		 * @param value The parameter is forwarded into the queue. The client must std::move() the item if they wish to transfer ownership.
		 */
		void push(StorageType&& value)
		{
			if (RWLock _ {_containerMutex}; true)
			{
				_counterAdds++;
				_container.push(std::forward<decltype(value)>(value));
			}
			// Must be outside the lock!
			_signal.release();
		}

		/**
		 * @brief Calls the underlying emplace method to the queue within a lock.
		 * 
		 * @param value The parameter is forwarded to the emplace method on the queue
		 */
		void emplace(StorageType&& value)
		{
			if (RWLock _ {_containerMutex}; true)
			{
				_counterAdds++;
				_container.emplace(std::forward<decltype(value)>(value));
			}
			// Must be outside the lock!
			_signal.release();
		}

		/**
		 * @brief Spins until the specified timeout to allow the outbound queue to be emptied.
         *        Use this call only when you're about to end use of the object and want the queue
         *        to be processed (without leaving unprocessed items).
		 * 
		 * @param timeoutDuration Timeout in milliseconds; defaults to 1500ms.
		 * @return std::optional<size_t> 
		 */
		auto waitUntilEmpty(std::chrono::milliseconds timeoutDuration = std::chrono::milliseconds(1500)) -> std::optional<size_t>
		{
			constexpr std::chrono::milliseconds spinInterval {32};
			std::chrono::milliseconds           spinDuration {32};

			while (!_container.empty() && (spinDuration < timeoutDuration))
			{
				// Something is in the queue.. let's spinwait
				std::this_thread::sleep_for(spinDuration);
				spinDuration += spinInterval;
			}

			return size();
		}


		/**
        * @brief Returns an item immediately otherwise waits for the minimum specified interval in milliseconds for an item to become available in the internal queue.
        * 
        * @param timeoutDuration 
        * @return std::optional<StorageType> 
        */
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
														   _container.pop();
														   _counterRemoves++;
													   }};
					// Return the object with minimal copy
					return std::make_optional(std::forward<StorageType>(_container.front()));
				}
			}

			// empty
			return {};
		}

		/**
         * @brief Returns the number of elements in the queue.
         * 
         * @return auto 
         */
		auto size() -> size_t const
		{
			RLock _ {_containerMutex};

			return _container.size();
		}

		/**
		 * @brief Returns the number of elements added thus far into the internal queue.
		 * 
		 * @return uint64_t 
		 */
		auto addCounter() -> uint64_t { return _counterAdds; }


		/**
		 * @brief Returns the number of times tryWaitItem resulted in a successful item retrieval.
		 * 
		 * @return uint64_t 
		 */
		auto removeCounter() -> uint64_t { return _counterRemoves; }

#ifdef INCLUDE_NLOHMANN_JSON_HPP_
	public:
		// If the JSON library is included in the current project, then make the serializer available.
		nlohmann::json toJson()
		{
			return nlohmann::json {{"_typver", "WaitableQueue/1.0.0"},
			                       {"adds", _counterAdds},
			                       {"removes", _counterRemoves},
			                       {"size", _container.size()}};
		}
#endif

	private:
		/// @brief Semaphore with default max signals.
		std::counting_semaphore<> _signal {0};
		/// @brief The container (defaults to std::queue)
		StorageContainer _container;
		/// @brief The shared mutex used to perform reader-writer lock
		mutable std::shared_mutex _containerMutex;
		/// @brief Tracks the total number of adds to the container
		uint64_t _counterAdds {0};
		/// @brief Tracks the total number of removes from the container
		uint64_t _counterRemoves {0};
	};
} // namespace siddiqsoft

#endif // !WaitableQueue_HPP
