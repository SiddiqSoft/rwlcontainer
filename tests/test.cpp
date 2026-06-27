/*
	RWLContainer : tests
	Reader Writer lock wrapper for object access
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
#include "nlohmann/json.hpp"
#include "../include/siddiqsoft/RWLContainer.hpp"


struct MyItem
{
	int         flag {0};
	std::string name {};
};

using MyItemPtr = std::shared_ptr<MyItem>;

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

TEST(RWContainer_add, BasicAdd)
{
	try
	{
		siddiqsoft::RWLContainer<std::string, MyItem> myContainer;

		auto item = myContainer.add("foo", {1, "bar"});
		ASSERT_TRUE(item);
		EXPECT_EQ("bar", item->name);
	}
	catch (...)
	{
		EXPECT_TRUE(false); // if we throw then the test fails.
	}
}

TEST(RWContainer_add, BasicAddINE_Collision)
{
	try
	{
		siddiqsoft::RWLContainer<std::string, MyItem> myContainer;

		auto item = myContainer.add("foo", {1, "bar"});
		ASSERT_TRUE(item);
		EXPECT_EQ("bar", item->name);
		auto item2 = myContainer.add(
				"foo", [&](auto& key) -> auto {
					EXPECT_FALSE(true) << "Should not be invoked";
					return std::make_shared<MyItem>(MyItem {0, key});
				});
		EXPECT_EQ(item, item2);
	}
	catch (...)
	{
		EXPECT_TRUE(false); // if we throw then the test fails.
	}
}

TEST(RWContainer_add, BasicAddINE_Ok)
{
	try
	{
		siddiqsoft::RWLContainer<std::string, MyItem> myContainer;

		auto item = myContainer.add("foo", {1, "bar"});
		ASSERT_TRUE(item);
		EXPECT_EQ("bar", item->name);
		auto item2 = myContainer.add(
				"good", [&](auto& key) -> auto {
					return std::make_shared<MyItem>(MyItem {2, key});
				});
		EXPECT_NE(item, item2);
		EXPECT_EQ("good", item2->name);
	}
	catch (...)
	{
		EXPECT_TRUE(false); // if we throw then the test fails.
	}
}

TEST(RWContainer_add, BasicAddFail)
{
	try
	{
		siddiqsoft::RWLContainer<std::string, MyItem> myContainer;

		auto item1 = myContainer.add("foo", {1, "bar"});
		ASSERT_TRUE(item1);
		EXPECT_EQ("bar", item1->name);

		myContainer.FailOnCollission = true;

		auto item2 = myContainer.add("foo", {2, "baz"});
		ASSERT_FALSE(item2);
	}
	catch (...)
	{
		EXPECT_TRUE(false); // if we throw then the test fails.
	}
}

TEST(RWContainer_add, BasicAddCollissionAllow)
{
	try
	{
		siddiqsoft::RWLContainer<std::string, MyItem> myContainer;
		// Add an item.
		auto item = myContainer.add("foo", {1, "bar"});
		ASSERT_TRUE(item);
		EXPECT_TRUE(item->name == "bar");
		// Add the item with new content for same key.
		myContainer.ReplaceExisting = true;
		auto item2                  = myContainer.add("foo", {2, "BAR"});
		EXPECT_TRUE(item2->name == "BAR");
		// We should have added with collission.
		EXPECT_TRUE(myContainer.size() == 1);
	}
	catch (...)
	{
		EXPECT_TRUE(false); // if we throw then the test fails.
	}
}


TEST(RWContainer_add, BasicCreateAndDestroy)
{
	try
	{
		siddiqsoft::RWLContainer<std::string, MyItem> myContainer;

		EXPECT_TRUE(true); // if we throw then the test fails.
	}
	catch (const std::exception& e)
	{
		EXPECT_TRUE(false) << e.what(); // if we throw then the test fails.
	}
}

TEST(RWContainer_diag, ToJSON)
{
	try
	{
		siddiqsoft::RWLContainer<std::string, MyItem> myContainer;

		nlohmann::json doc = myContainer.toJson();
		std::cerr << doc.dump() << std::endl;
		EXPECT_TRUE(doc.size() > 0); // if we throw then the test fails.
	}
	catch (...)
	{
		EXPECT_TRUE(false); // if we throw then the test fails.
	}
}

TEST(RWContainer_find, BasicFindOKLargeSet)
{
	try
	{
		const int                                    ITEM_COUNT    = 20000;
		auto                                         hasCollission = false;
		siddiqsoft::RWLContainer<std::string, MyItem> myContainer;

		// Add an item.
		{
			for (auto i = 0; i < ITEM_COUNT; i++)
			{
				auto item = myContainer.add(std::string("foo_").append(std::to_string(i + 1)), {i, "bar"});
				EXPECT_TRUE(item);
			}
			// Add an item.
			auto item = myContainer.add("FOO", {1, "bard"});
			EXPECT_TRUE(item);
			EXPECT_TRUE(item->name == "bard");
		}

		{
			auto myItem = myContainer.find("FOO");
			EXPECT_TRUE(myItem);
			EXPECT_TRUE(myItem->name == "bard");
		}
	}
	catch (...)
	{
		EXPECT_TRUE(false); // if we throw then the test fails.
	}
}

TEST(RWContainer_find, BasicFindNegative)
{
	try
	{
		siddiqsoft::RWLContainer<std::string, MyItem> myContainer;

		// Add an item.
		auto item = myContainer.add("foo", {1, "bar"});
		EXPECT_TRUE(item->name == "bar");
		auto myItem = myContainer.find("-not-exist-");
		EXPECT_FALSE(myItem);
	}
	catch (...)
	{
		EXPECT_TRUE(false); // if we throw then the test fails.
	}
}


TEST(RWContainer_remove, BasicRemoveOK)
{
	try
	{
		siddiqsoft::RWLContainer<std::string, MyItem> myContainer;

		// Add an item.
		auto item = myContainer.add("foo", {1, "bar"});
		EXPECT_TRUE(item);
		EXPECT_TRUE(item->name == "bar");

		// Remove the item.
		auto myItem = myContainer.remove("foo");
		EXPECT_TRUE(myItem);
		EXPECT_TRUE(myItem->name == "bar");
	}
	catch (...)
	{
		EXPECT_TRUE(false); // if we throw then the test fails.
	}
}

TEST(RWContainer_remove, BasicRemoveNegative)
{
	try
	{
		auto                                         hasCollission = false;
		siddiqsoft::RWLContainer<std::string, MyItem> myContainer;

		// Add an item.
		auto item = myContainer.add("foo", {1, "bar"});
		EXPECT_TRUE(item->name == "bar");

		// Remove the item.
		auto myItem = myContainer.remove("not-found");
		EXPECT_FALSE(myItem);
	}
	catch (...)
	{
		EXPECT_TRUE(false); // if we throw then the test fails.
	}
}


TEST(RWContainer_scan, BasicScanBuildJSONOK)
{
	const int      ITEM_COUNT = 20000;
	nlohmann::json doc;

	try
	{
		siddiqsoft::RWLContainer<std::string, MyItem> myContainer;
		// Add a bunch of items.
		{
			for (auto i = 0; i < ITEM_COUNT; i++)
			{
				auto item = myContainer.add(std::string("foo_").append(std::to_string(i + 1)), {i, "bar"});
				EXPECT_TRUE(item);
			}
		}
		EXPECT_EQ(ITEM_COUNT, myContainer.size());

		{
			myContainer.scan([&doc](const auto& key, auto& val) -> bool {
				// we're already inside the lock
				// build our json.. basically, we're performing a copy
				doc.push_back({{"id", key}, {"flag", val->flag}, {"name", val->name}});
				return false;
			});
		}

		EXPECT_EQ(ITEM_COUNT, doc.size());
	}
	catch (...)
	{
		EXPECT_TRUE(false); // if we throw then the test fails.
	}
}

TEST(RWContainer_scan, BasicScanOK)
{
	const int ITEM_COUNT    = 2;
	int       scanIteration = 0;
	uint64_t  ttxAdd {0};
	uint64_t  ttxScan {0};

	try
	{
		siddiqsoft::RWLContainer<std::string, MyItem> myContainer;
		// Add a bunch of items.
		{
			for (auto i = 0; i < ITEM_COUNT; i++)
			{
				auto item = myContainer.add(std::string("foo_").append(std::to_string(i + 1)), {i, "bar"});
				EXPECT_TRUE(item);
			}
		}
		EXPECT_TRUE(myContainer.size() == ITEM_COUNT);

		// Add item at the end to force a scan for this in the custom find.
		auto item = myContainer.add("UNIQUE-ITEM", {9999999, "uniquebar"});
		EXPECT_TRUE(item);

		{
			auto myItem = myContainer.scan([&](const auto& key, auto& val) -> bool {
				// we're already inside the lock
				scanIteration++;
				return (key == "UNIQUE-ITEM") && (val->flag == 9999999);
			});
			EXPECT_TRUE(myItem);
			EXPECT_TRUE(myItem->name == "uniquebar");
		}
	}
	catch (...)
	{
		EXPECT_TRUE(false); // if we throw then the test fails.
	}
}

TEST(RWContainer_scan, BasicScanNegative)
{
	try
	{
		const int                                    ITEM_COUNT = 20000;
		uint64_t                                     ttxAdd {0};
		uint64_t                                     ttxScan {0};
		auto                                         hasCollission = false;
		siddiqsoft::RWLContainer<std::string, MyItem> myContainer;

		// Add a bunch of items.
		{
			for (auto i = 0; i < ITEM_COUNT; i++)
			{
				auto item = myContainer.add(std::string("foo_").append(std::to_string(i + 1)), {i, "bar"});
				EXPECT_TRUE(item);
			}
		}
		EXPECT_TRUE(myContainer.size() == ITEM_COUNT);

		// This will in effect scan the entire list without finding anything.
		{
			auto myItem = myContainer.scan([](const auto& key, auto& val) -> bool {
				// we're already inside the lock
				return key == "-not-found-";
			});
			EXPECT_FALSE(myItem); // should not find anything
		}
	}
	catch (...)
	{
		EXPECT_TRUE(false); // if we throw then the test fails.
	}
}


// ============================================================================
// Additional coverage tests for RWLContainer
// ============================================================================

// --- Tests for add(key, StorageTypePtr&&) overload ---

TEST(RWContainer_add, AddSharedPtr)
{
	siddiqsoft::RWLContainer<std::string, MyItem> myContainer;

	auto ptr  = std::make_shared<MyItem>(MyItem {42, "shared"});
	auto item = myContainer.add("key1", std::move(ptr));
	ASSERT_TRUE(item);
	EXPECT_EQ("shared", item->name);
	EXPECT_EQ(42, item->flag);
	EXPECT_EQ(1u, myContainer.size());
}

TEST(RWContainer_add, AddSharedPtr_CollisionReturnsExisting)
{
	siddiqsoft::RWLContainer<std::string, MyItem> myContainer;

	auto ptr1  = std::make_shared<MyItem>(MyItem {1, "first"});
	auto item1 = myContainer.add("key1", std::move(ptr1));
	ASSERT_TRUE(item1);

	// Default: FailOnCollission=false, ReplaceExisting=false => returns existing
	auto ptr2  = std::make_shared<MyItem>(MyItem {2, "second"});
	auto item2 = myContainer.add("key1", std::move(ptr2));
	ASSERT_TRUE(item2);
	EXPECT_EQ("first", item2->name); // should be the original
	EXPECT_EQ(item1, item2);
	EXPECT_EQ(1u, myContainer.size());
}

TEST(RWContainer_add, AddSharedPtr_FailOnCollision)
{
	siddiqsoft::RWLContainer<std::string, MyItem> myContainer;
	myContainer.FailOnCollission = true;

	auto ptr1  = std::make_shared<MyItem>(MyItem {1, "first"});
	auto item1 = myContainer.add("key1", std::move(ptr1));
	ASSERT_TRUE(item1);

	auto ptr2  = std::make_shared<MyItem>(MyItem {2, "second"});
	auto item2 = myContainer.add("key1", std::move(ptr2));
	EXPECT_FALSE(item2); // collision => nullptr
	EXPECT_EQ(1u, myContainer.size());
}

TEST(RWContainer_add, AddSharedPtr_ReplaceExisting)
{
	siddiqsoft::RWLContainer<std::string, MyItem> myContainer;
	myContainer.ReplaceExisting = true;

	auto ptr1  = std::make_shared<MyItem>(MyItem {1, "first"});
	auto item1 = myContainer.add("key1", std::move(ptr1));
	ASSERT_TRUE(item1);
	EXPECT_EQ("first", item1->name);

	auto ptr2  = std::make_shared<MyItem>(MyItem {2, "replaced"});
	auto item2 = myContainer.add("key1", std::move(ptr2));
	ASSERT_TRUE(item2);
	EXPECT_EQ("replaced", item2->name);
	EXPECT_EQ(1u, myContainer.size());
}

// --- Tests for add(key, callback) with FailOnCollission and ReplaceExisting ---

TEST(RWContainer_add, AddCallback_FailOnCollision)
{
	siddiqsoft::RWLContainer<std::string, MyItem> myContainer;
	myContainer.FailOnCollission = true;

	auto item1 = myContainer.add("key1", MyItem {1, "first"});
	ASSERT_TRUE(item1);

	bool callbackInvoked = false;
	auto item2           = myContainer.add(
            "key1",
            [&](const std::string& key) -> MyItemPtr {
                callbackInvoked = true;
                return std::make_shared<MyItem>(MyItem {2, key});
            });
	EXPECT_FALSE(item2);          // collision => nullptr
	EXPECT_FALSE(callbackInvoked); // callback should NOT be invoked
}

TEST(RWContainer_add, AddCallback_ReplaceExisting)
{
	siddiqsoft::RWLContainer<std::string, MyItem> myContainer;
	myContainer.ReplaceExisting = true;

	auto item1 = myContainer.add("key1", MyItem {1, "first"});
	ASSERT_TRUE(item1);

	bool callbackInvoked = false;
	auto item2           = myContainer.add(
            "key1",
            [&](const std::string& key) -> MyItemPtr {
                callbackInvoked = true;
                return std::make_shared<MyItem>(MyItem {99, "replaced-via-callback"});
            });
	ASSERT_TRUE(item2);
	EXPECT_TRUE(callbackInvoked);
	EXPECT_EQ("replaced-via-callback", item2->name);
	EXPECT_EQ(99, item2->flag);
	EXPECT_EQ(1u, myContainer.size());
}

// --- Empty container operations ---

TEST(RWContainer_find, FindOnEmptyContainer)
{
	siddiqsoft::RWLContainer<std::string, MyItem> myContainer;
	auto                                          item = myContainer.find("nonexistent");
	EXPECT_FALSE(item);
}

TEST(RWContainer_remove, RemoveOnEmptyContainer)
{
	siddiqsoft::RWLContainer<std::string, MyItem> myContainer;
	auto                                          item = myContainer.remove("nonexistent");
	EXPECT_FALSE(item);
}

TEST(RWContainer_scan, ScanOnEmptyContainer)
{
	siddiqsoft::RWLContainer<std::string, MyItem> myContainer;
	bool                                          callbackInvoked = false;
	auto                                          item            = myContainer.scan([&](const auto& key, auto& val) -> bool {
        callbackInvoked = true;
        return true;
    });
	EXPECT_FALSE(item);
	EXPECT_FALSE(callbackInvoked);
}

TEST(RWContainer_size, SizeOnEmptyContainer)
{
	siddiqsoft::RWLContainer<std::string, MyItem> myContainer;
	EXPECT_EQ(0u, myContainer.size());
}

TEST(RWContainer_size, SizeAfterAddAndRemove)
{
	siddiqsoft::RWLContainer<std::string, MyItem> myContainer;
	myContainer.add("a", MyItem {1, "alpha"});
	myContainer.add("b", MyItem {2, "beta"});
	EXPECT_EQ(2u, myContainer.size());

	[[maybe_unused]] auto ra = myContainer.remove("a");
	EXPECT_EQ(1u, myContainer.size());

	[[maybe_unused]] auto rb = myContainer.remove("b");
	EXPECT_EQ(0u, myContainer.size());
}

// --- Using std::map as StorageContainer ---

TEST(RWContainer_map, AddFindRemoveWithOrderedMap)
{
	siddiqsoft::RWLContainer<std::string, MyItem, std::map<std::string, std::shared_ptr<MyItem>>> myContainer;

	auto item1 = myContainer.add("charlie", MyItem {3, "Charlie"});
	auto item2 = myContainer.add("alpha", MyItem {1, "Alpha"});
	auto item3 = myContainer.add("bravo", MyItem {2, "Bravo"});

	ASSERT_TRUE(item1);
	ASSERT_TRUE(item2);
	ASSERT_TRUE(item3);
	EXPECT_EQ(3u, myContainer.size());

	auto found = myContainer.find("bravo");
	ASSERT_TRUE(found);
	EXPECT_EQ("Bravo", found->name);

	auto removed = myContainer.remove("alpha");
	ASSERT_TRUE(removed);
	EXPECT_EQ("Alpha", removed->name);
	EXPECT_EQ(2u, myContainer.size());

	// Scan should work with ordered map
	std::vector<std::string> keys;
	myContainer.scan([&](const auto& key, auto& val) -> bool {
		keys.push_back(key);
		return false;
	});
	EXPECT_EQ(2u, keys.size());
	// std::map iterates in sorted order
	EXPECT_EQ("bravo", keys[0]);
	EXPECT_EQ("charlie", keys[1]);
}

// --- toJson with counters after operations ---

TEST(RWContainer_diag, ToJSON_AfterOperations)
{
	siddiqsoft::RWLContainer<std::string, MyItem> myContainer;

	myContainer.add("a", MyItem {1, "alpha"});
	myContainer.add("b", MyItem {2, "beta"});
	myContainer.add("c", MyItem {3, "gamma"});
	[[maybe_unused]] auto rb = myContainer.remove("b");

	nlohmann::json doc = myContainer.toJson();
	std::cerr << doc.dump() << std::endl;

	EXPECT_EQ(3u, doc["adds"].get<uint64_t>());
	EXPECT_EQ(1u, doc["removes"].get<uint64_t>());
	EXPECT_EQ(2u, doc["size"].get<uint64_t>());
	EXPECT_FALSE(doc["ReplaceExisting"].get<bool>());
	EXPECT_FALSE(doc["FailOnCollission"].get<bool>());
}

// --- Multiple removes of same key ---

TEST(RWContainer_remove, DoubleRemoveSameKey)
{
	siddiqsoft::RWLContainer<std::string, MyItem> myContainer;
	myContainer.add("key1", MyItem {1, "value"});

	auto first  = myContainer.remove("key1");
	ASSERT_TRUE(first);
	EXPECT_EQ("value", first->name);

	auto second = myContainer.remove("key1");
	EXPECT_FALSE(second); // already removed
	EXPECT_EQ(0u, myContainer.size());
}

// --- Add after remove (re-add) ---

TEST(RWContainer_add, ReAddAfterRemove)
{
	siddiqsoft::RWLContainer<std::string, MyItem> myContainer;

	myContainer.add("key1", MyItem {1, "original"});
	[[maybe_unused]] auto removed = myContainer.remove("key1");
	EXPECT_EQ(0u, myContainer.size());

	auto item = myContainer.add("key1", MyItem {2, "re-added"});
	ASSERT_TRUE(item);
	EXPECT_EQ("re-added", item->name);
	EXPECT_EQ(2, item->flag);
	EXPECT_EQ(1u, myContainer.size());
}

// --- Scan that finds the first matching item ---

TEST(RWContainer_scan, ScanFindsFirstMatch)
{
	siddiqsoft::RWLContainer<std::string, MyItem> myContainer;
	myContainer.add("a", MyItem {1, "alpha"});
	myContainer.add("b", MyItem {2, "beta"});
	myContainer.add("c", MyItem {3, "gamma"});

	int scanCount = 0;
	auto item     = myContainer.scan([&](const auto& key, auto& val) -> bool {
        scanCount++;
        return val->flag == 2; // find the item with flag==2
    });
	ASSERT_TRUE(item);
	EXPECT_EQ("beta", item->name);
	// scanCount depends on iteration order (unordered_map), but item must be found
}

// --- Integer key type ---

TEST(RWContainer_add, IntegerKeyType)
{
	siddiqsoft::RWLContainer<int, MyItem> myContainer;

	auto item1 = myContainer.add(1, MyItem {10, "one"});
	auto item2 = myContainer.add(2, MyItem {20, "two"});
	auto item3 = myContainer.add(3, MyItem {30, "three"});

	ASSERT_TRUE(item1);
	ASSERT_TRUE(item2);
	ASSERT_TRUE(item3);
	EXPECT_EQ(3u, myContainer.size());

	auto found = myContainer.find(2);
	ASSERT_TRUE(found);
	EXPECT_EQ("two", found->name);

	auto removed = myContainer.remove(1);
	ASSERT_TRUE(removed);
	EXPECT_EQ("one", removed->name);
	EXPECT_EQ(2u, myContainer.size());
}

// --- ReplaceExisting + FailOnCollission both true: FailOnCollission takes precedence ---

TEST(RWContainer_add, FailOnCollisionTakesPrecedenceOverReplace)
{
	siddiqsoft::RWLContainer<std::string, MyItem> myContainer;
	myContainer.FailOnCollission = true;
	myContainer.ReplaceExisting  = true;

	auto item1 = myContainer.add("key1", MyItem {1, "first"});
	ASSERT_TRUE(item1);

	// FailOnCollission is checked first in the code, so collision should fail
	auto item2 = myContainer.add("key1", MyItem {2, "second"});
	EXPECT_FALSE(item2);
	EXPECT_EQ(1u, myContainer.size());

	// Verify original is unchanged
	auto found = myContainer.find("key1");
	ASSERT_TRUE(found);
	EXPECT_EQ("first", found->name);
}

// --- Large number of adds and removes to verify counter accuracy ---

TEST(RWContainer_diag, CounterAccuracy)
{
	siddiqsoft::RWLContainer<std::string, MyItem> myContainer;

	const int ADD_COUNT    = 500;
	const int REMOVE_COUNT = 200;

	for (int i = 0; i < ADD_COUNT; ++i)
	{
		myContainer.add(std::format("key_{}", i), MyItem {i, std::format("item_{}", i)});
	}

	for (int i = 0; i < REMOVE_COUNT; ++i)
	{
		[[maybe_unused]] auto r = myContainer.remove(std::format("key_{}", i));
	}

	nlohmann::json doc = myContainer.toJson();
	EXPECT_EQ(static_cast<uint64_t>(ADD_COUNT), doc["adds"].get<uint64_t>());
	EXPECT_EQ(static_cast<uint64_t>(REMOVE_COUNT), doc["removes"].get<uint64_t>());
	EXPECT_EQ(static_cast<uint64_t>(ADD_COUNT - REMOVE_COUNT), doc["size"].get<uint64_t>());
}

// --- All three add overloads with shared_ptr on same container ---

TEST(RWContainer_add, AllThreeOverloads)
{
	siddiqsoft::RWLContainer<std::string, MyItem> myContainer;

	// Overload 1: add(key, StorageType&&)
	auto item1 = myContainer.add("val", MyItem {1, "by-value"});
	ASSERT_TRUE(item1);
	EXPECT_EQ("by-value", item1->name);

	// Overload 2: add(key, StorageTypePtr&&)
	auto ptr   = std::make_shared<MyItem>(MyItem {2, "by-ptr"});
	auto item2 = myContainer.add("ptr", std::move(ptr));
	ASSERT_TRUE(item2);
	EXPECT_EQ("by-ptr", item2->name);

	// Overload 3: add(key, callback)
	auto item3 = myContainer.add(
			"cb", [](const std::string& key) -> MyItemPtr { return std::make_shared<MyItem>(MyItem {3, "by-callback"}); });
	ASSERT_TRUE(item3);
	EXPECT_EQ("by-callback", item3->name);

	EXPECT_EQ(3u, myContainer.size());
}

// NOLINTEND(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
