/*
	Reader-Writer lock protected container

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
#ifndef RWLCONTAINER_HPP
#define RWLCONTAINER_HPP

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
	/// @brief Thread-safe dictionary container with reader-writer locking
	///
	/// Implements an unordered map container with reader-writer locking for efficient concurrent access.
	/// Multiple threads can read simultaneously, while writes are exclusive. All values are stored as
	/// shared_ptr for automatic memory management.
	///
	/// @tparam KeyType The key type (e.g., std::string, int)
	/// @tparam StorageType The value type to store (avoid pointers; use value types)
	/// @tparam StorageContainer The underlying container type (defaults to std::unordered_map)
	///
	/// @note Thread-safe: All operations are protected by std::shared_mutex
	/// @note Values are stored as shared_ptr<StorageType> for automatic memory management
	/// @note Configuration flags: ReplaceExisting, FailOnCollission
	///
	/// @example
	/// @code
	/// siddiqsoft::RWLContainer<std::string, int> cache;
	/// cache.add("key1", 42);
	/// auto value = cache.find("key1");
	/// if (value) { std::cout << *value << std::endl; }
	/// @endcode
	template <class KeyType,
	          class StorageType,
	          typename StorageContainer = std::unordered_map<KeyType, std::shared_ptr<StorageType>>>
	class RWLContainer
	{
	public:
		using StorageTypePtr = std::shared_ptr<StorageType>;


	private:
		StorageContainer          _container {};
		mutable std::shared_mutex _containerMutex {};
		std::atomic_uint64_t      _counterAdds {};
		std::atomic_uint64_t      _counterRemoves {};

	public:
		/// @brief If true, replaces existing items on add; if false, returns existing item
		bool ReplaceExisting {false};

		/// @brief If true, fails (returns empty shared_ptr) on key collision; if false, returns existing item
		bool FailOnCollission {false};

		/// @brief Adds an element by moving a value into a shared_ptr
		///
		/// Adds or updates an element in the container. The value is moved into a shared_ptr
		/// for automatic memory management. Behavior on key collision is controlled by
		/// ReplaceExisting and FailOnCollission flags.
		///
		/// @param key The key to associate with the value
		/// @param value The value to store (moved into shared_ptr)
		/// @return shared_ptr to the newly inserted or existing item, or empty shared_ptr on collision with FailOnCollission=true
		///
		/// @note Thread-safe: Uses exclusive write lock
		/// @note Collision behavior:
		///   - FailOnCollission=true: Returns empty shared_ptr
		///   - ReplaceExisting=true: Replaces existing value
		///   - Otherwise: Returns existing value
		///
		/// @throws std::runtime_error if insertion fails
		///
		/// @example
		/// @code
		/// auto item = container.add("key1", MyType{42, "value"});
		/// @endcode
		StorageTypePtr add(const KeyType& key, StorageType&& value)
		{
			std::unique_lock<std::shared_mutex> myWriterLock(_containerMutex);

			// Search for any existing item..
			auto itemFound = _container.find(key);
			if (itemFound != _container.end() && FailOnCollission)
				return {};
			else if (itemFound != _container.end() && !FailOnCollission && !ReplaceExisting)
				return itemFound->second;

			// Item not found.. ReplaceExisting=> true and FailOnCollission=> false
			if (auto [iter, rv] = _container.insert_or_assign(key, std::make_shared<StorageType>(std::move(value)));
			    iter != _container.end())            // insert/assign new item
				return _counterAdds++, iter->second; // shortcut expression

			throw std::runtime_error(std::format("{} - Failed to add for key:{}", __FUNCTION__, key));
		}

		/// @brief Adds an element by moving an existing shared_ptr
		///
		/// Adds or updates an element in the container using an existing shared_ptr.
		/// This overload is useful when you already have a shared_ptr and want to transfer ownership.
		/// Behavior on key collision is controlled by ReplaceExisting and FailOnCollission flags.
		///
		/// @param key The key to associate with the value
		/// @param value The shared_ptr to store (moved into container)
		/// @return shared_ptr to the newly inserted or existing item, or empty shared_ptr on collision with FailOnCollission=true
		///
		/// @note Thread-safe: Uses exclusive write lock
		/// @note Collision behavior same as add(key, StorageType&&)
		/// @note The input shared_ptr is moved; original reference becomes invalid
		///
		/// @throws std::runtime_error if insertion fails
		///
		/// @example
		/// @code
		/// auto ptr = std::make_shared<MyType>(42, "value");
		/// auto item = container.add("key1", std::move(ptr));
		/// @endcode
		StorageTypePtr add(const KeyType& key, StorageTypePtr&& value)
		{
			std::unique_lock<std::shared_mutex> myWriterLock(_containerMutex);

			// Search for any existing item..
			auto itemFound = _container.find(key);
			if (itemFound != _container.end() && FailOnCollission)
				return {};
			else if (itemFound != _container.end() && !FailOnCollission && !ReplaceExisting)
				return itemFound->second;

			// Item not found.. ReplaceExisting=> true and FailOnCollission=> false
			if (auto [iter, rv] = _container.insert_or_assign(key, std::move(value));
			    iter != _container.end())                            // insert/assign new item
				return _counterAdds++ ? iter->second : iter->second; // shortcut expression

			throw std::runtime_error(std::format("{} - Failed to add for key:{}", __FUNCTION__, key));
		}

		/// @brief Adds an element via callback for lazy initialization
		///
		/// Adds an element by invoking a callback function if the key doesn't exist.
		/// If the key already exists, returns the existing value without invoking the callback.
		/// Useful for lazy initialization patterns.
		///
		/// @param key The key to add a new item
		/// @param newObjectCallback Callback function that accepts the key and returns a shared_ptr<StorageType>.
		///                          Only invoked if key doesn't exist (or ReplaceExisting=true).
		/// @return shared_ptr to the newly created or existing item, or empty shared_ptr on collision with FailOnCollission=true
		///
		/// @note Thread-safe: Uses exclusive write lock
		/// @note The callback executes within the write lock, so keep it brief
		/// @note Collision behavior same as add(key, StorageType&&)
		/// @note Useful for lazy initialization: container.add("key", [](const auto& k) { return std::make_shared<MyType>(k); })
		///
		/// @throws std::runtime_error if insertion fails
		///
		/// @example
		/// @code
		/// auto item = container.add("key1", [](const auto& key) {
		///     return std::make_shared<MyType>(key);
		/// });
		/// @endcode
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

		/// @brief Removes and returns an element by key
		///
		/// Removes an element from the container and returns it. If the key doesn't exist,
		/// returns an empty shared_ptr. The removed item is returned to the caller, allowing
		/// for cleanup or further processing.
		///
		/// @param key The key to remove
		/// @return shared_ptr to the removed item, or empty shared_ptr if key not found
		///
		/// @note Thread-safe: Uses exclusive write lock
		/// @note The returned shared_ptr keeps the item alive even after removal from container
		/// @note Increments the remove counter
		/// @note [[nodiscard]] attribute encourages checking the return value
		///
		/// @example
		/// @code
		/// auto removed = container.remove("key1");
		/// if (removed) {
		///     std::cout << "Removed item" << std::endl;
		/// }
		/// @endcode
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

		/// @brief Finds and returns an element by key without removing it
		///
		/// Searches for an element in the container and returns it if found.
		/// The element remains in the container. This is a read-only operation.
		///
		/// @param key The key to find
		/// @return shared_ptr to the found item, or empty shared_ptr if key not found
		///
		/// @note Thread-safe: Uses shared read lock (allows concurrent reads)
		/// @note Does not modify the container
		/// @note Multiple threads can call find() simultaneously
		///
		/// @example
		/// @code
		/// auto item = container.find("key1");
		/// if (item) {
		///     std::cout << "Found: " << *item << std::endl;
		/// }
		/// @endcode
		StorageTypePtr find(const KeyType& key)
		{
			std::shared_lock<std::shared_mutex> myReaderLock(_containerMutex);

			if (auto item = _container.find(key); item != _container.end()) return item->second;

			return {};
		}

		/// @brief Returns the number of elements in the container
		///
		/// Gets the current size of the container. This is a read-only operation.
		///
		/// @return The number of elements currently in the container
		///
		/// @note Thread-safe: Uses shared read lock
		/// @note O(1) operation
		/// @note Multiple threads can call size() simultaneously
		///
		/// @example
		/// @code
		/// std::cout << "Container has " << container.size() << " items" << std::endl;
		/// @endcode
		auto size() const -> size_t const
		{
			std::shared_lock<std::shared_mutex> myReaderLock(_containerMutex);

			return _container.size();
		}

		/// @brief Iterates through all elements and returns the first match
		///
		/// Scans through all elements in the container, calling the callback for each one.
		/// Returns the first element where the callback returns true, or empty shared_ptr if no match.
		/// Useful for custom search logic or filtering.
		///
		/// @param scanCallback Callback function that receives (key, value) and returns true to stop iteration
		/// @return shared_ptr to the first matching item, or empty shared_ptr if no match found
		///
		/// @note Thread-safe: Uses shared read lock (entire scan is atomic)
		/// @note The callback is invoked for each element until it returns true
		/// @note Callback receives const reference to key and reference to value
		/// @note Multiple threads can call scan() simultaneously
		///
		/// @example
		/// @code
		/// auto item = container.scan([](const auto& key, auto& value) {
		///     return key.find("target") != std::string::npos;
		/// });
		/// @endcode
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
		/// @brief Serializes container metadata to JSON
		///
		/// Returns a JSON object containing container statistics and configuration.
		/// Only available if nlohmann/json.hpp is included before this header.
		///
		/// @return JSON object with:
		///   - _typver: Type and version string
		///   - adds: Total number of add operations
		///   - removes: Total number of remove operations
		///   - ReplaceExisting: Current configuration flag
		///   - FailOnCollission: Current configuration flag
		///   - size: Current number of items
		///
		/// @note Thread-safe: Uses shared read lock
		/// @note Requires INCLUDE_NLOHMANN_JSON_HPP_ to be defined
		///
		/// @example
		/// @code
		/// #include "nlohmann/json.hpp"
		/// auto json = container.toJson();
		/// std::cout << json.dump(2) << std::endl;
		/// @endcode
		auto toJson() const -> nlohmann::json const
		{
			return nlohmann::json {{"_typver", "RWLContainer/1.5.3"},
			                       {"adds", _counterAdds.load()},
			                       {"removes", _counterRemoves.load()},
			                       {"ReplaceExisting", ReplaceExisting},
			                       {"FailOnCollission", FailOnCollission},
			                       {"size", size()}};
		}
#endif
	};
} // namespace siddiqsoft

#endif // !RWLCONTAINER_HPP
