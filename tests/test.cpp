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

#include "nlohmann/json.hpp"
#include "../src/RWLContainer.hpp"


struct MyItem
{
	int         flag {0};
	std::string name {};
};

using MyItemPtr = std::shared_ptr<MyItem>;

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
