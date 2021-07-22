/*
	Reader-Writer protected container

	BSD 3-Clause License

	Copyright (c) 2021, Siddiq Software LLC
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
#ifndef RWCONTAINER_HPP
#define RWCONTAINER_HPP

#include <map>
#include <unordered_map>
#include <functional>
#include <memory>
#include <string>
#include <atomic>
#include <shared_mutex>
#include <type_traits>

namespace siddiqsoft
{
	/// @brief Implements an unordered map container with reader-writer locking. The internal storage is via shared_ptr so the
	/// @tparam StorageType Can be any element but avoid using pointers, shared_ptr, unique_ptr as the underlying storage is shared_ptr<StorageType>
	template <class KeyType,
	          class StorageType,
	          typename StorageContainer = std::unordered_map<KeyType, std::shared_ptr<StorageType>>>
	class RWContainer
	{
	public:
		using StorageTypePtr = std::shared_ptr<StorageType>;

		bool ReplaceExisting {false};
		bool FailOnCollission {false};

		/// @brief Adds an element by taking ownership and storing it within a shared_ptr
		/// @param key Case-sensitive string
		/// @param value Non pointer
		/// @return The newly inserted item or existing item
		StorageTypePtr add(const KeyType& key, const StorageType&& value)
		{
			std::unique_lock<std::shared_mutex> myWriterLock(_containerMutex);

			// Search for any existing item..
			auto itemFound = _container.find(key);
			if (itemFound != _container.end() && FailOnCollission)
				return {};
			else if (itemFound != _container.end() && !FailOnCollission && !ReplaceExisting)
				return itemFound->second;

			// Item not found.. ReplaceExisting=> true and FailOnCollission=> false
			if (auto [iter, rv] = _container.insert_or_assign(key, std::make_shared<StorageType>(value));
			    iter != _container.end())                            // insert/assign new item
				return _counterAdds++ ? iter->second : iter->second; // shortcut expression

			throw std::runtime_error(std::format("{} - Failed to add for key:{}", __FUNCTION__, key));
		}


		/// @brief Adds an element by taking ownership and storing it within a shared_ptr
		/// @param key Case-sensitive string
		/// @param value shared_ptr
		/// @return The newly inserted item or existing item
		StorageTypePtr add(const KeyType& key, const StorageTypePtr&& value)
		{
			std::unique_lock<std::shared_mutex> myWriterLock(_containerMutex);

			// Search for any existing item..
			auto itemFound = _container.find(key);
			if (itemFound != _container.end() && FailOnCollission)
				return {};
			else if (itemFound != _container.end() && !FailOnCollission && !ReplaceExisting)
				return itemFound->second;

			// Item not found.. ReplaceExisting=> true and FailOnCollission=> false
			if (auto [iter, rv] = _container.insert_or_assign(key, value); iter != _container.end()) // insert/assign new item
				return _counterAdds++ ? iter->second : iter->second;                                 // shortcut expression

			throw std::runtime_error(std::format("{} - Failed to add for key:{}", __FUNCTION__, key));
		}


		/// @brief This method allows for adding of items via callback if the item does not exist
		/// If an existing item is found, it is returned without invoking the callback
		/// @param key The key to add a new itme
		/// @param newObjectCallback Callback accepts key and returns the shared_ptr object to add associated with the key.
		/// WARNING! The add happens within a lock!
		/// @return The newly created object or an existing object associated with the key.
		StorageTypePtr add(const KeyType& key, std::function<StorageTypePtr(const KeyType&)>&& newObjectCallback)
		{
			std::unique_lock<std::shared_mutex> myWriterLock(_containerMutex);

			// Search for any existing item..
			auto itemFound = _container.find(key);
			if (itemFound != _container.end() && FailOnCollission)
				return {}; // collission but asked to fail
			else if (itemFound != _container.end() && !FailOnCollission && !ReplaceExisting)
				return itemFound->second; // found existing; return

			// Item not found.. ReplaceExisting=> true and FailOnCollission=> false
			if (auto [iter, rv] = _container.insert_or_assign(key, newObjectCallback(key));
			    iter != _container.end())                            // insert/assign new item
				return _counterAdds++ ? iter->second : iter->second; // shortcut expression; returns added item

			throw std::runtime_error(std::format("{} - Failed to add for key:{}", __FUNCTION__, key));
		}


		[[nodiscard]] StorageTypePtr remove(const KeyType& key)
		{
			std::unique_lock<std::shared_mutex> myWriterLock(_containerMutex);

			// Search for any existing item..
			if (auto item = _container.find(key); item != _container.end())
			{
				auto retItem = item->second;

				_container.erase(item);
				_counterRemoves++;

				return retItem;
			}

			return {};
		}


		StorageTypePtr find(const KeyType& key)
		{
			std::shared_lock<std::shared_mutex> myReaderLock(_containerMutex);

			if (auto item = _container.find(key); item != _container.end()) return item->second;

			return {};
		}


		auto size() const
		{
			std::shared_lock<std::shared_mutex> myReaderLock(_containerMutex);

			return _container.size();
		}


		StorageTypePtr scan(std::function<bool(const KeyType&, StorageTypePtr&)> scanCallback)
		{
			std::shared_lock<std::shared_mutex> myReaderLock(_containerMutex);

			for (auto& item : _container)
			{
				if (scanCallback(item.first, item.second)) return item.second;
			}

			return {};
		}

#ifdef INCLUDE_NLOHMANN_JSON_HPP_
	public:
		// If the JSON library is included in the current project, then make the serializer available.
		nlohmann::json toJson()
		{
			return nlohmann::json {{"_typver", "RWContainer/1.0.0"},
			                       {"adds", _counterAdds.load()},
			                       {"removes", _counterRemoves.load()},
			                       {"ReplaceExisting", ReplaceExisting},
			                       {"FailOnCollission", FailOnCollission},
			                       {"size", _container.size()}};
		}
#endif

	private:
		StorageContainer          _container {};
		mutable std::shared_mutex _containerMutex {};
		std::atomic_uint64_t      _counterAdds {};
		std::atomic_uint64_t      _counterRemoves {};
	};
} // namespace siddiqsoft

#endif // !RWCONTAINER_HPP
