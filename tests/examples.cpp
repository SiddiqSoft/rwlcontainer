#include "gtest/gtest.h"
#include <cstdint>
#include <format>
#include <queue>
#include <unordered_map>
#include <thread>
#include <array>

#include "../include/siddiqsoft/RWLContainer.hpp"
#include "../include/siddiqsoft/RWLQueue.hpp"

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

TEST(multithread, QueueAndEmit)
{
	static const int                  THREAD_COUNT = 6;
	siddiqsoft::RWLQueue<std::string> myContainer;

	auto workerFunction = [&myContainer](std::stop_token st)
	{
		using namespace std::chrono_literals;
		uint64_t itemCount = 0;
		std::cout << "Worker started." << std::endl;
		while (!st.stop_requested())
		{
			auto item = myContainer.getNext();
			itemCount++;
			//if (item) { std::cout << *item << std::endl; }
		}
		std::cout << "Worker ended. " << itemCount << std::endl;
	};

	std::array<std::jthread, THREAD_COUNT> threadPool {};
	for (int i = 0; i < THREAD_COUNT; i++)
	{
		threadPool[i] = std::jthread(workerFunction);
	}

	for (auto i = 0; i < 10000; i++)
	{
		myContainer.push(std::format("Item: {}", i));
	}

	EXPECT_EQ(10000, myContainer.addCounter()) << myContainer.size();
}
