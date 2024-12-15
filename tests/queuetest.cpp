
#include "gtest/gtest.h"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <format>
#include <functional>
#include <queue>
#include <string>
#include <unordered_map>
#include <thread>
#include <array>
#include <optional>
#include <format>

#include "../include/siddiqsoft/RWLContainer.hpp"
#include "../include/siddiqsoft/WaitableQueue.hpp"

static std::atomic_uint64_t CountObjectsDestroyed {0};

struct MyTestObject
{
	int                        id;
	std::string                name;
	std::optional<std::string> description;

public:
	MyTestObject(const std::string& n, const std::string& d = {})
		: name(n)
	{
		if (!d.empty()) description = d;
	}

	~MyTestObject()
	{
		CountObjectsDestroyed++;
#if defined(DEBUG) || defined(_DEBUG)
		std::cout << std::format("Destroyed >>{},{}<< {} Counter:{}\n",
		                         id,
		                         name,
		                         (description.has_value() ? *description : ""),
		                         CountObjectsDestroyed.load());
#endif
	}
};

TEST(WaitableQueueTests, CustomObject)
{
	siddiqsoft::WaitableQueue<MyTestObject> myContainer1;
	myContainer1.push(MyTestObject {"my name"});
}

TEST(WaitableQueueTests, LoadTest_1)
{
	CountObjectsDestroyed                                  = 0;
	static const auto                      ITERATION_COUNT = 910000;
	static const int                       THREAD_COUNT    = 4;
	siddiqsoft::WaitableQueue<std::string> myContainer;

	try
	{
		// The worker function for each thread..
		auto workerFunction = [](std::stop_token st, siddiqsoft::WaitableQueue<std::string>& myContainer)
		{
			uint64_t itemCount = 0;

			std::cout << std::this_thread::get_id() << " - LoadTest_1 Worker started." << std::endl;
			while (!st.stop_requested())
			{
				auto item = myContainer.tryWaitItem();
				if (item.has_value()) itemCount++;
			}
			std::cout << std::this_thread::get_id() << " - LoadTest_1 Worker ended..." << itemCount << std::endl;
		};

		// Create the workers..
		std::array<std::jthread, THREAD_COUNT> threadPool {};
		for (int i = 0; i < THREAD_COUNT; i++)
		{
			threadPool[i] = std::jthread(workerFunction, std::ref(myContainer));
		}

		for (auto i = 0; i < ITERATION_COUNT; i++)
		{
			myContainer.emplace(std::format("Item---------------------------: {}", i));
			if (i > (ITERATION_COUNT / THREAD_COUNT)) threadPool[THREAD_COUNT - 1].request_stop();
			if (i > (ITERATION_COUNT / THREAD_COUNT - 1)) threadPool[THREAD_COUNT - 2].request_stop();
			if (i > (ITERATION_COUNT / THREAD_COUNT - 2)) threadPool[THREAD_COUNT - 3].request_stop();
		}

		// Immediately request stop..
		for (auto& t : threadPool)
		{
			std::cout << "Force stopping thread #" << t.get_id() << std::endl;
			t.request_stop();
		}
	}
	catch (...)
	{
	}

	std::cout << std::format("{} - Expected Iteration count: {}  Adds: {}  Destroyed: {}\n",
	                         __func__,
	                         ITERATION_COUNT,
	                         myContainer.addCounter(),
	                         CountObjectsDestroyed.load());
	EXPECT_EQ(ITERATION_COUNT, myContainer.addCounter()) << myContainer.size();
}

TEST(WaitableQueueTests, LoadTest_2)
{
	CountObjectsDestroyed                                   = 0;
	static const auto                       ITERATION_COUNT = 10;
	static const int                        THREAD_COUNT    = 4;
	siddiqsoft::WaitableQueue<MyTestObject> myContainer;

	try
	{
		// The worker function for each thread..
		auto workerFunction = [](std::stop_token st, siddiqsoft::WaitableQueue<MyTestObject>& myContainer)
		{
			uint64_t itemCount = 0;

			std::cout << std::this_thread::get_id() << " - LoadTest_2 Worker started." << std::endl;
			while (!st.stop_requested())
			{
				auto item = myContainer.tryWaitItem();
				if (item.has_value()) itemCount++;
			}
			std::cout << std::this_thread::get_id() << " - LoadTest_2 Worker ended..." << itemCount << std::endl;
		};

		// Create the workers..
		std::array<std::jthread, THREAD_COUNT> threadPool {};
		for (int i = 0; i < THREAD_COUNT; i++)
		{
			threadPool[i] = std::jthread(workerFunction, std::ref(myContainer));
		}

		for (auto i = 0; i < ITERATION_COUNT; i++)
		{
			myContainer.emplace(MyTestObject(std::format("MyObject(2):{}:{}", i, ITERATION_COUNT)));
		}

		// Wait until we're done spooling all items..
		while (myContainer.size() > 0)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		// Immediately request stop..
		for (auto& t : threadPool)
		{
			std::cout << "LoadTest_2 - Force stopping thread #" << t.get_id() << std::endl;
			t.request_stop();
		}
	}
	catch (...)
	{
	}

	std::cout << std::format("\n{} - Expected Iteration count: {}  Adds: {}  Removes: {}  size: {}   Destroyed: {} ++--++\n",
	                         __func__,
	                         ITERATION_COUNT,
	                         myContainer.addCounter(),
	                         myContainer.removeCounter(),
	                         myContainer.size(),
	                         CountObjectsDestroyed.load());
	EXPECT_EQ(ITERATION_COUNT, myContainer.addCounter()) << myContainer.size();
	EXPECT_EQ(ITERATION_COUNT, myContainer.removeCounter()) << myContainer.size();
}

TEST(WaitableQueueTests, LoadTest_Fail)
{
	CountObjectsDestroyed                                   = 0;
	static const auto                       ITERATION_COUNT = 10;
	static const int                        THREAD_COUNT    = 4;
	siddiqsoft::WaitableQueue<MyTestObject> myContainer;

	try
	{
		// The worker function for each thread..
		auto workerFunction = [](std::stop_token st, siddiqsoft::WaitableQueue<MyTestObject>& myContainer)
		{
			uint64_t itemCount = 0;

			std::cout << std::this_thread::get_id() << " - LoadTest_Fail Worker started." << std::endl;
			while (!st.stop_requested())
			{
				auto item = myContainer.tryWaitItem();
				if (item.has_value()) itemCount++;
			}
			std::cout << std::this_thread::get_id() << " - LoadTest_Fail Worker ended..." << itemCount << std::endl;
		};

		// Create the workers..
		std::array<std::jthread, THREAD_COUNT> threadPool {};
		for (int i = 0; i < THREAD_COUNT; i++)
		{
			threadPool[i] = std::jthread(workerFunction, std::ref(myContainer));
		}

		for (auto i = 0; i < ITERATION_COUNT; i++)
		{
			myContainer.emplace(MyTestObject(std::format("MyObject(3):{}:{}", i, ITERATION_COUNT)));
			if (i > (ITERATION_COUNT / THREAD_COUNT)) threadPool[THREAD_COUNT - 1].request_stop();
			if (i > (ITERATION_COUNT / THREAD_COUNT - 1)) threadPool[THREAD_COUNT - 2].request_stop();
			if (i > (ITERATION_COUNT / THREAD_COUNT - 2)) threadPool[THREAD_COUNT - 3].request_stop();
			if (i > (ITERATION_COUNT / THREAD_COUNT - 3)) threadPool[THREAD_COUNT - 4].request_stop();
		}

		// Immediately request stop..
		// This SHOULD leave some items unprocessed in the pool
		for (auto& t : threadPool)
		{
			std::cout << "LoadTest_Fail - Force stopping thread #" << t.get_id() << std::endl;
			t.request_stop();
		}
	}
	catch (...)
	{
	}

	std::cout << std::format("\n{} - Expected Iteration count: {}  Adds: {}  Removes: {}  size: {}   Destroyed: {} ++--++\n",
	                         __func__,
	                         ITERATION_COUNT,
	                         myContainer.addCounter(),
	                         myContainer.removeCounter(),
	                         myContainer.size(),
	                         CountObjectsDestroyed.load());
	EXPECT_EQ(ITERATION_COUNT, myContainer.addCounter()) << myContainer.size();
	EXPECT_NE(ITERATION_COUNT, myContainer.removeCounter()) << myContainer.size();
}
