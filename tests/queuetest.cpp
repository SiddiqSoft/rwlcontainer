#include "gtest/gtest.h"
#include <cstdint>
#include <format>
#include <queue>
#include <unordered_map>
#include <thread>
#include <array>

#include "../include/siddiqsoft/RWLContainer.hpp"
#include "../include/siddiqsoft/RWLQueue.hpp"


TEST(WaitableQueue, Copy)
{
	
}

TEST(WaitableQueue, LoadAndSpool)
{
	static const int                  THREAD_COUNT = 6;
	siddiqsoft::RWLQueue<std::string> myContainer;

	// The worker function for each thread..
	auto workerFunction = [&myContainer](std::stop_token st)
	{
		uint64_t itemCount = 0;
		
		std::cout << std::this_thread::get_id() <<  " - Worker started." << std::endl;
		while (!st.stop_requested())
		{
			auto item = myContainer.getNext();
			itemCount++;
		}
		std::cout << std::this_thread::get_id() <<  " - Worker ended..." << itemCount << std::endl;
	};

	// Create the workers..
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
