#include "gtest/gtest.h"

#include <unordered_map>
#include "../src/RWLContainer.hpp"


TEST(examples, Example1)
{
	try
	{
		siddiqsoft::RWLContainer<std::string, std::string> myContainer;

		auto item = myContainer.add("foo", "bar");
		ASSERT_TRUE(item);
		EXPECT_EQ("bar", *item);
	}
	catch (...)
	{
		EXPECT_TRUE(false); // if we throw then the test fails.
	}
}