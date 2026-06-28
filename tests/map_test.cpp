/*
	RWLContainer with std::map storage tests
	Reader Writer lock wrapper for object access with ordered map storage
	Repo: https://github.com/SiddiqSoft/RWLContainer

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

#include "gtest/gtest.h"
#include <format>
#include <map>
#include "nlohmann/json.hpp"
#include "../include/siddiqsoft/RWLContainer.hpp"

struct MapTestItem
{
	int         id {0};
	std::string value {};
	double      score {0.0};
};

using MapTestItemPtr = std::shared_ptr<MapTestItem>;

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

// ============================================================================
// Basic Operations with std::map Storage
// ============================================================================

TEST(RWContainer_map, BasicAddWithMap)
{
	siddiqsoft::RWLContainer<std::string, MapTestItem, std::map<std::string, std::shared_ptr<MapTestItem>>> myContainer;

	auto item = myContainer.add("key1", MapTestItem {.id = 1, .value = "test", .score = 9.5});
	ASSERT_TRUE(item);
	EXPECT_EQ(1, item->id);
	EXPECT_EQ("test", item->value);
	EXPECT_EQ(9.5, item->score);
	EXPECT_EQ(1u, myContainer.size());
}

TEST(RWContainer_map, BasicFindWithMap)
{
	siddiqsoft::RWLContainer<std::string, MapTestItem, std::map<std::string, std::shared_ptr<MapTestItem>>> myContainer;

	myContainer.add("alpha", MapTestItem {.id = 1, .value = "first", .score = 1.0});
	myContainer.add("beta", MapTestItem {.id = 2, .value = "second", .score = 2.0});
	myContainer.add("gamma", MapTestItem {.id = 3, .value = "third", .score = 3.0});

	auto found = myContainer.find("beta");
	ASSERT_TRUE(found);
	EXPECT_EQ(2, found->id);
	EXPECT_EQ("second", found->value);
}

TEST(RWContainer_map, BasicRemoveWithMap)
{
	siddiqsoft::RWLContainer<std::string, MapTestItem, std::map<std::string, std::shared_ptr<MapTestItem>>> myContainer;

	myContainer.add("key1", MapTestItem {.id = 1, .value = "item1", .score = 1.0});
	myContainer.add("key2", MapTestItem {.id = 2, .value = "item2", .score = 2.0});
	EXPECT_EQ(2u, myContainer.size());

	auto removed = myContainer.remove("key1");
	ASSERT_TRUE(removed);
	EXPECT_EQ(1, removed->id);
	EXPECT_EQ(1u, myContainer.size());

	auto notFound = myContainer.find("key1");
	EXPECT_FALSE(notFound);
}

TEST(RWContainer_map, FindNonexistentWithMap)
{
	siddiqsoft::RWLContainer<std::string, MapTestItem, std::map<std::string, std::shared_ptr<MapTestItem>>> myContainer;

	myContainer.add("exists", MapTestItem {.id = 1, .value = "value", .score = 1.0});

	auto notFound = myContainer.find("nonexistent");
	EXPECT_FALSE(notFound);
}

TEST(RWContainer_map, RemoveNonexistentWithMap)
{
	siddiqsoft::RWLContainer<std::string, MapTestItem, std::map<std::string, std::shared_ptr<MapTestItem>>> myContainer;

	myContainer.add("exists", MapTestItem {.id = 1, .value = "value", .score = 1.0});

	auto notRemoved = myContainer.remove("nonexistent");
	EXPECT_FALSE(notRemoved);
	EXPECT_EQ(1u, myContainer.size());
}

// ============================================================================
// Ordered Iteration with std::map
// ============================================================================

TEST(RWContainer_map, OrderedIterationWithMap)
{
	siddiqsoft::RWLContainer<std::string, MapTestItem, std::map<std::string, std::shared_ptr<MapTestItem>>> myContainer;

	// Add items in non-alphabetical order
	myContainer.add("zebra", MapTestItem {.id = 3, .value = "z", .score = 3.0});
	myContainer.add("apple", MapTestItem {.id = 1, .value = "a", .score = 1.0});
	myContainer.add("mango", MapTestItem {.id = 2, .value = "m", .score = 2.0});

	// Scan should iterate in sorted order (map property)
	std::vector<std::string> keys;
	myContainer.scan(
			[&](const auto& key, auto& val) -> bool
			{
				keys.push_back(key);
				return false;
			});

	EXPECT_EQ(3u, keys.size());
	EXPECT_EQ("apple", keys[0]);
	EXPECT_EQ("mango", keys[1]);
	EXPECT_EQ("zebra", keys[2]);
}

TEST(RWContainer_map, ScanCollectsAllItemsInOrder)
{
	siddiqsoft::RWLContainer<std::string, MapTestItem, std::map<std::string, std::shared_ptr<MapTestItem>>> myContainer;

	// Add items
	for (int i = 0; i < 10; ++i)
	{
		std::string key = std::format("item_{:02d}", i);
		myContainer.add(key, MapTestItem {.id = i, .value = key, .score = static_cast<double>(i)});
	}

	// Collect all items via scan
	std::vector<MapTestItem> items;
	myContainer.scan(
			[&](const auto& key, auto& val) -> bool
			{
				items.push_back(*val);
				return false;
			});

	EXPECT_EQ(10u, items.size());
	// Verify they're in order
	for (size_t i = 0; i < items.size(); ++i)
	{
		EXPECT_EQ(static_cast<int>(i), items[i].id);
	}
}

TEST(RWContainer_map, ScanFindFirstMatchInOrder)
{
	siddiqsoft::RWLContainer<std::string, MapTestItem, std::map<std::string, std::shared_ptr<MapTestItem>>> myContainer;

	myContainer.add("a", MapTestItem {.id = 1, .value = "alpha", .score = 1.0});
	myContainer.add("b", MapTestItem {.id = 2, .value = "beta", .score = 2.0});
	myContainer.add("c", MapTestItem {.id = 3, .value = "gamma", .score = 3.0});
	myContainer.add("d", MapTestItem {.id = 4, .value = "delta", .score = 4.0});

	// Find item with score > 2.5
	auto found = myContainer.scan(
			[](const auto& key, auto& val) -> bool
			{
				return val->score > 2.5;
			});

	ASSERT_TRUE(found);
	EXPECT_EQ(3, found->id); // Should find gamma (first match in order)
}

// ============================================================================
// Collision Handling with std::map
// ============================================================================

TEST(RWContainer_map, CollisionDefaultBehaviorWithMap)
{
	siddiqsoft::RWLContainer<std::string, MapTestItem, std::map<std::string, std::shared_ptr<MapTestItem>>> myContainer;

	auto item1 = myContainer.add("key", MapTestItem {.id = 1, .value = "first", .score = 1.0});
	ASSERT_TRUE(item1);

	// Default: returns existing item
	auto item2 = myContainer.add("key", MapTestItem {.id = 2, .value = "second", .score = 2.0});
	ASSERT_TRUE(item2);
	EXPECT_EQ(item1, item2);
	EXPECT_EQ(1, item2->id); // Should be the original
	EXPECT_EQ(1u, myContainer.size());
}

TEST(RWContainer_map, CollisionFailOnCollisionWithMap)
{
	siddiqsoft::RWLContainer<std::string, MapTestItem, std::map<std::string, std::shared_ptr<MapTestItem>>> myContainer;
	myContainer.FailOnCollission = true;

	auto item1 = myContainer.add("key", MapTestItem {.id = 1, .value = "first", .score = 1.0});
	ASSERT_TRUE(item1);

	auto item2 = myContainer.add("key", MapTestItem {.id = 2, .value = "second", .score = 2.0});
	EXPECT_FALSE(item2); // Should fail
	EXPECT_EQ(1u, myContainer.size());
}

TEST(RWContainer_map, CollisionReplaceExistingWithMap)
{
	siddiqsoft::RWLContainer<std::string, MapTestItem, std::map<std::string, std::shared_ptr<MapTestItem>>> myContainer;
	myContainer.ReplaceExisting = true;

	auto item1 = myContainer.add("key", MapTestItem {.id = 1, .value = "first", .score = 1.0});
	ASSERT_TRUE(item1);
	EXPECT_EQ(1, item1->id);

	auto item2 = myContainer.add("key", MapTestItem {.id = 2, .value = "second", .score = 2.0});
	ASSERT_TRUE(item2);
	EXPECT_EQ(2, item2->id); // Should be the new value
	EXPECT_EQ(1u, myContainer.size());
}

// ============================================================================
// Shared Pointer Overload with std::map
// ============================================================================

TEST(RWContainer_map, AddSharedPtrWithMap)
{
	siddiqsoft::RWLContainer<std::string, MapTestItem, std::map<std::string, std::shared_ptr<MapTestItem>>> myContainer;

	auto ptr  = std::make_shared<MapTestItem>(MapTestItem {.id = 42, .value = "shared", .score = 4.2});
	auto item = myContainer.add("key1", std::move(ptr));
	ASSERT_TRUE(item);
	EXPECT_EQ(42, item->id);
	EXPECT_EQ("shared", item->value);
	EXPECT_EQ(1u, myContainer.size());
}

TEST(RWContainer_map, AddSharedPtrCollisionWithMap)
{
	siddiqsoft::RWLContainer<std::string, MapTestItem, std::map<std::string, std::shared_ptr<MapTestItem>>> myContainer;
	myContainer.ReplaceExisting = true;

	auto ptr1  = std::make_shared<MapTestItem>(MapTestItem {.id = 1, .value = "first", .score = 1.0});
	auto item1 = myContainer.add("key", std::move(ptr1));
	ASSERT_TRUE(item1);

	auto ptr2  = std::make_shared<MapTestItem>(MapTestItem {.id = 2, .value = "replaced", .score = 2.0});
	auto item2 = myContainer.add("key", std::move(ptr2));
	ASSERT_TRUE(item2);
	EXPECT_EQ(2, item2->id);
	EXPECT_EQ("replaced", item2->value);
	EXPECT_EQ(1u, myContainer.size());
}

// ============================================================================
// Callback Overload with std::map
// ============================================================================

TEST(RWContainer_map, AddCallbackWithMap)
{
	siddiqsoft::RWLContainer<std::string, MapTestItem, std::map<std::string, std::shared_ptr<MapTestItem>>> myContainer;

	auto item = myContainer.add("key1",
	                            [](const std::string& key) -> MapTestItemPtr
	                            { return std::make_shared<MapTestItem>(MapTestItem {.id = 99, .value = key, .score = 9.9}); });
	ASSERT_TRUE(item);
	EXPECT_EQ(99, item->id);
	EXPECT_EQ("key1", item->value);
	EXPECT_EQ(1u, myContainer.size());
}

TEST(RWContainer_map, AddCallbackNotInvokedOnCollisionWithMap)
{
	siddiqsoft::RWLContainer<std::string, MapTestItem, std::map<std::string, std::shared_ptr<MapTestItem>>> myContainer;

	auto item1 = myContainer.add("key", MapTestItem {.id = 1, .value = "first", .score = 1.0});
	ASSERT_TRUE(item1);

	bool callbackInvoked = false;
	auto item2           = myContainer.add("key",
	                                       [&](const std::string& key) -> MapTestItemPtr
	                                       {
										 callbackInvoked = true;
										 return std::make_shared<MapTestItem>(MapTestItem {.id = 2, .value = key, .score = 2.0});
									   });
	ASSERT_TRUE(item2);
	EXPECT_FALSE(callbackInvoked); // Callback should not be invoked (existing item returned)
	EXPECT_EQ(1, item2->id);       // Should be the original
}

TEST(RWContainer_map, AddCallbackInvokedOnReplaceWithMap)
{
	siddiqsoft::RWLContainer<std::string, MapTestItem, std::map<std::string, std::shared_ptr<MapTestItem>>> myContainer;
	myContainer.ReplaceExisting = true;

	auto item1 = myContainer.add("key", MapTestItem {.id = 1, .value = "first", .score = 1.0});
	ASSERT_TRUE(item1);

	bool callbackInvoked = false;
	auto item2           = myContainer.add("key",
	                                       [&](const std::string& key) -> MapTestItemPtr
	                                       {
										 callbackInvoked = true;
										 return std::make_shared<MapTestItem>(MapTestItem {.id = 2, .value = "replaced", .score = 2.0});
									   });
	ASSERT_TRUE(item2);
	EXPECT_TRUE(callbackInvoked);  // Callback should be invoked
	EXPECT_EQ(2, item2->id);       // Should be the new value
	EXPECT_EQ("replaced", item2->value);
}

// ============================================================================
// Size and Empty Container Operations with std::map
// ============================================================================

TEST(RWContainer_map, SizeWithMap)
{
	siddiqsoft::RWLContainer<std::string, MapTestItem, std::map<std::string, std::shared_ptr<MapTestItem>>> myContainer;

	EXPECT_EQ(0u, myContainer.size());

	myContainer.add("a", MapTestItem {.id = 1, .value = "a", .score = 1.0});
	EXPECT_EQ(1u, myContainer.size());

	myContainer.add("b", MapTestItem {.id = 2, .value = "b", .score = 2.0});
	EXPECT_EQ(2u, myContainer.size());

	myContainer.remove("a");
	EXPECT_EQ(1u, myContainer.size());

	myContainer.remove("b");
	EXPECT_EQ(0u, myContainer.size());
}

TEST(RWContainer_map, EmptyContainerOperationsWithMap)
{
	siddiqsoft::RWLContainer<std::string, MapTestItem, std::map<std::string, std::shared_ptr<MapTestItem>>> myContainer;

	EXPECT_EQ(0u, myContainer.size());
	EXPECT_FALSE(myContainer.find("any"));
	EXPECT_FALSE(myContainer.remove("any"));

	bool callbackInvoked = false;
	auto result          = myContainer.scan(
			[&](const auto& key, auto& val) -> bool
			{
				callbackInvoked = true;
				return true;
			});
	EXPECT_FALSE(result);
	EXPECT_FALSE(callbackInvoked);
}

// ============================================================================
// JSON Serialization with std::map
// ============================================================================

TEST(RWContainer_map, ToJsonWithMap)
{
	siddiqsoft::RWLContainer<std::string, MapTestItem, std::map<std::string, std::shared_ptr<MapTestItem>>> myContainer;

	myContainer.add("a", MapTestItem {.id = 1, .value = "alpha", .score = 1.0});
	myContainer.add("b", MapTestItem {.id = 2, .value = "beta", .score = 2.0});

	nlohmann::json doc = myContainer.toJson();
	EXPECT_EQ(2u, doc["adds"].get<uint64_t>());
	EXPECT_EQ(0u, doc["removes"].get<uint64_t>());
	EXPECT_EQ(2u, doc["size"].get<uint64_t>());
	EXPECT_FALSE(doc["ReplaceExisting"].get<bool>());
	EXPECT_FALSE(doc["FailOnCollission"].get<bool>());
}

TEST(RWContainer_map, ToJsonAfterOperationsWithMap)
{
	siddiqsoft::RWLContainer<std::string, MapTestItem, std::map<std::string, std::shared_ptr<MapTestItem>>> myContainer;

	myContainer.add("a", MapTestItem {.id = 1, .value = "alpha", .score = 1.0});
	myContainer.add("b", MapTestItem {.id = 2, .value = "beta", .score = 2.0});
	myContainer.add("c", MapTestItem {.id = 3, .value = "gamma", .score = 3.0});
	myContainer.remove("b");

	nlohmann::json doc = myContainer.toJson();
	EXPECT_EQ(3u, doc["adds"].get<uint64_t>());
	EXPECT_EQ(1u, doc["removes"].get<uint64_t>());
	EXPECT_EQ(2u, doc["size"].get<uint64_t>());
}

// ============================================================================
// Large Dataset Operations with std::map
// ============================================================================

TEST(RWContainer_map, LargeDatasetWithMap)
{
	siddiqsoft::RWLContainer<std::string, MapTestItem, std::map<std::string, std::shared_ptr<MapTestItem>>> myContainer;

	const int ITEM_COUNT = 1000;

	// Add items
	for (int i = 0; i < ITEM_COUNT; ++i)
	{
		std::string key = std::format("item_{:04d}", i);
		myContainer.add(key, MapTestItem {.id = i, .value = key, .score = static_cast<double>(i)});
	}

	EXPECT_EQ(static_cast<size_t>(ITEM_COUNT), myContainer.size());

	// Find specific items
	auto found = myContainer.find("item_0500");
	ASSERT_TRUE(found);
	EXPECT_EQ(500, found->id);

	// Remove some items
	for (int i = 0; i < 100; ++i)
	{
		std::string key = std::format("item_{:04d}", i);
		myContainer.remove(key);
	}

	EXPECT_EQ(static_cast<size_t>(ITEM_COUNT - 100), myContainer.size());
}

TEST(RWContainer_map, LargeDatasetScanWithMap)
{
	siddiqsoft::RWLContainer<std::string, MapTestItem, std::map<std::string, std::shared_ptr<MapTestItem>>> myContainer;

	const int ITEM_COUNT = 500;

	for (int i = 0; i < ITEM_COUNT; ++i)
	{
		std::string key = std::format("item_{:04d}", i);
		myContainer.add(key, MapTestItem {.id = i, .value = key, .score = static_cast<double>(i)});
	}

	// Scan and collect all items
	std::vector<int> ids;
	myContainer.scan(
			[&](const auto& key, auto& val) -> bool
			{
				ids.push_back(val->id);
				return false;
			});

	EXPECT_EQ(static_cast<size_t>(ITEM_COUNT), ids.size());

	// Verify they're in sorted order (map property)
	for (size_t i = 0; i < ids.size(); ++i)
	{
		EXPECT_EQ(static_cast<int>(i), ids[i]);
	}
}

// ============================================================================
// Integer Key Type with std::map
// ============================================================================

TEST(RWContainer_map, IntegerKeyWithMap)
{
	siddiqsoft::RWLContainer<int, MapTestItem, std::map<int, std::shared_ptr<MapTestItem>>> myContainer;

	myContainer.add(3, MapTestItem {.id = 3, .value = "three", .score = 3.0});
	myContainer.add(1, MapTestItem {.id = 1, .value = "one", .score = 1.0});
	myContainer.add(2, MapTestItem {.id = 2, .value = "two", .score = 2.0});

	EXPECT_EQ(3u, myContainer.size());

	auto found = myContainer.find(2);
	ASSERT_TRUE(found);
	EXPECT_EQ("two", found->value);

	// Scan should iterate in sorted order (1, 2, 3)
	std::vector<int> keys;
	myContainer.scan(
			[&](const auto& key, auto& val) -> bool
			{
				keys.push_back(key);
				return false;
			});

	EXPECT_EQ(3u, keys.size());
	EXPECT_EQ(1, keys[0]);
	EXPECT_EQ(2, keys[1]);
	EXPECT_EQ(3, keys[2]);
}

// ============================================================================
// Re-add After Remove with std::map
// ============================================================================

TEST(RWContainer_map, ReAddAfterRemoveWithMap)
{
	siddiqsoft::RWLContainer<std::string, MapTestItem, std::map<std::string, std::shared_ptr<MapTestItem>>> myContainer;

	auto item1 = myContainer.add("key", MapTestItem {.id = 1, .value = "first", .score = 1.0});
	ASSERT_TRUE(item1);

	auto removed = myContainer.remove("key");
	ASSERT_TRUE(removed);
	EXPECT_EQ(0u, myContainer.size());

	auto item2 = myContainer.add("key", MapTestItem {.id = 2, .value = "second", .score = 2.0});
	ASSERT_TRUE(item2);
	EXPECT_EQ(2, item2->id);
	EXPECT_EQ("second", item2->value);
	EXPECT_EQ(1u, myContainer.size());
}

// ============================================================================
// Multiple Removes with std::map
// ============================================================================

TEST(RWContainer_map, MultipleRemovesWithMap)
{
	siddiqsoft::RWLContainer<std::string, MapTestItem, std::map<std::string, std::shared_ptr<MapTestItem>>> myContainer;

	myContainer.add("a", MapTestItem {.id = 1, .value = "a", .score = 1.0});
	myContainer.add("b", MapTestItem {.id = 2, .value = "b", .score = 2.0});
	myContainer.add("c", MapTestItem {.id = 3, .value = "c", .score = 3.0});

	auto removed_a = myContainer.remove("a");
	ASSERT_TRUE(removed_a);
	EXPECT_EQ(2u, myContainer.size());

	auto removed_b = myContainer.remove("b");
	ASSERT_TRUE(removed_b);
	EXPECT_EQ(1u, myContainer.size());

	auto removed_c = myContainer.remove("c");
	ASSERT_TRUE(removed_c);
	EXPECT_EQ(0u, myContainer.size());

	// Try to remove again
	auto not_found = myContainer.remove("a");
	EXPECT_FALSE(not_found);
}

// ============================================================================
// Configuration Flags with std::map
// ============================================================================

TEST(RWContainer_map, ConfigurationFlagsWithMap)
{
	siddiqsoft::RWLContainer<std::string, MapTestItem, std::map<std::string, std::shared_ptr<MapTestItem>>> myContainer;

	EXPECT_FALSE(myContainer.ReplaceExisting);
	EXPECT_FALSE(myContainer.FailOnCollission);

	myContainer.ReplaceExisting = true;
	EXPECT_TRUE(myContainer.ReplaceExisting);

	myContainer.FailOnCollission = true;
	EXPECT_TRUE(myContainer.FailOnCollission);

	// FailOnCollission takes precedence
	auto item1 = myContainer.add("key", MapTestItem {.id = 1, .value = "first", .score = 1.0});
	ASSERT_TRUE(item1);

	auto item2 = myContainer.add("key", MapTestItem {.id = 2, .value = "second", .score = 2.0});
	EXPECT_FALSE(item2); // FailOnCollission takes precedence
}

// ============================================================================
// Stress Test with std::map
// ============================================================================

TEST(RWContainer_map, StressTestWithMap)
{
	siddiqsoft::RWLContainer<std::string, MapTestItem, std::map<std::string, std::shared_ptr<MapTestItem>>> myContainer;

	const int OPERATIONS = 100;

	// Add many items
	for (int i = 0; i < OPERATIONS; ++i)
	{
		std::string key = std::format("stress_{:03d}", i);
		myContainer.add(key, MapTestItem {.id = i, .value = key, .score = static_cast<double>(i)});
	}

	EXPECT_EQ(static_cast<size_t>(OPERATIONS), myContainer.size());

	// Find random items
	for (int i = 0; i < 10; ++i)
	{
		std::string key = std::format("stress_{:03d}", i * 10);
		auto found      = myContainer.find(key);
		ASSERT_TRUE(found);
		EXPECT_EQ(i * 10, found->id);
	}

	// Remove every other item
	for (int i = 0; i < OPERATIONS; i += 2)
	{
		std::string key = std::format("stress_{:03d}", i);
		auto removed    = myContainer.remove(key);
		ASSERT_TRUE(removed);
	}

	EXPECT_EQ(static_cast<size_t>(OPERATIONS / 2), myContainer.size());

	// Verify remaining items
	int count = 0;
	myContainer.scan(
			[&](const auto& key, auto& val) -> bool
			{
				count++;
				return false;
			});

	EXPECT_EQ(static_cast<size_t>(OPERATIONS / 2), static_cast<size_t>(count));
}

// NOLINTEND(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
