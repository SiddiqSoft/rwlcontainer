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

#ifndef RWLQUEUE_HPP
#define RWLQUEUE_HPP


#include <concepts>
#include <queue>
#include <functional>
#include <memory>
#include <string>
#include <atomic>
#include <shared_mutex>
#include <semaphore>
#include <type_traits>

namespace siddiqsoft
{
	/// @brief Implements a queue container with reader-writer locking.
	/// @tparam StorageType Can be any element but avoid using pointers, shared_ptr, unique_ptr
	template <typename StorageType>
		requires std::move_constructible<StorageType>
	struct RWLQueue
	{
		using RWLock = std::unique_lock<std::shared_mutex>;
		using RLock  = std::shared_lock<std::shared_mutex>;

	public:
		~RWLQueue() { _signal.release(); }
		//RWLQueue(RWLQueue&&)       = delete;
		//auto operator=(RWLQueue&&) = delete;

		/// @brief Adds an element by taking ownership and storing it within a shared_ptr
		/// @param key Case-sensitive string
		/// @param value Non pointer
		/// @return The newly inserted item or existing item
		void push(StorageType&& value)
		{
			{ // scoped read and write lock
				RWLock _ {_containerMutex};
				_counterAdds++;
				_container.push(std::move(value));
			}
			// Must be outside the lock!
			_signal.release();
		}


		[[nodiscard]] auto getNext() -> std::optional<StorageType>
		{
			using namespace std::chrono_literals;

			if (_signal.try_acquire_for(500ms))
			{
				RWLock _ {_containerMutex};

				if (!_container.empty())
				{
					StorageType item {std::move(_container.front())};
					_container.pop();
					_counterRemoves++;
					return std::move(item);
				}
			}

			// empty
			return {};
		}


		auto size() const
		{
			RLock _ {_containerMutex};

			return _container.size();
		}

		auto addCounter() -> uint64_t { return _counterAdds.load(); }

		auto removeCounter() -> uint64_t { return _counterRemoves.load(); }

#ifdef INCLUDE_NLOHMANN_JSON_HPP_
	public:
		// If the JSON library is included in the current project, then make the serializer available.
		nlohmann::json toJson()
		{
			return nlohmann::json {{"_typver", "RWLQueue/1.0.0"},
			                       {"adds", _counterAdds.load()},
			                       {"removes", _counterRemoves.load()},
			                       {"size", _container.size()}};
		}
#endif

	private:
		/// @brief Semaphore with default max signals.
		std::counting_semaphore<> _signal {0};
		std::queue<StorageType>   _container {};
		mutable std::shared_mutex _containerMutex {};
		std::atomic_uint64_t      _counterAdds {};
		std::atomic_uint64_t      _counterRemoves {};
	};
} // namespace siddiqsoft

#endif // !RWLQUEUE_HPP
