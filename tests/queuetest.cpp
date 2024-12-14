#include "gtest/gtest.h"
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

struct MyTestObject
{
	int                        id {__COUNTER__};
	std::string                name {"empty"};
	std::optional<std::string> description {"empty-description"};

public:
	MyTestObject()
	{
		name = std::to_string(__COUNTER__);
		name.append(":empty");
	}
	MyTestObject(const std::string& n, const std::string& d = {})
	{
		if (!d.empty()) description = d;
		name = std::to_string(__COUNTER__);
		name.append(":").append(n);
	}
	MyTestObject(MyTestObject&&) = default;

	~MyTestObject()
	{
#if defined(DEBUG) || defined(_DEBUG)
		std::cout << "Destroyed >>" << id << "," << name << "<< " << (description.has_value() ? *description : "") << std::endl;
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
	static const auto                      ITERATION_COUNT = 910000;
	static const int                       THREAD_COUNT    = 4;
	siddiqsoft::WaitableQueue<std::string> myContainer;

	// The worker function for each thread..
	auto workerFunction = [](std::stop_token st, siddiqsoft::WaitableQueue<std::string>& myContainer)
	{
		uint64_t itemCount = 0;

		std::cout << std::this_thread::get_id() << " - Worker started." << std::endl;
		while (!st.stop_requested())
		{
			auto item = myContainer.tryWaitForNextItem();
			if (item.has_value()) itemCount++;
		}
		std::cout << std::this_thread::get_id() << " - Worker ended..." << itemCount << std::endl;
	};

	// Create the workers..
	std::array<std::jthread, THREAD_COUNT> threadPool {};
	for (int i = 0; i < THREAD_COUNT; i++)
	{
		threadPool[i] = std::jthread(workerFunction, std::ref(myContainer));
	}

	for (auto i = 0; i < ITERATION_COUNT; i++)
	{
		myContainer.push(std::move(std::format("Item: {}", i)));
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

	EXPECT_EQ(ITERATION_COUNT, myContainer.addCounter()) << myContainer.size();
}

TEST(WaitableQueueTests, LoadTest_2)
{
	static const auto                       ITERATION_COUNT = 1000;
	static const int                        THREAD_COUNT    = 4;
	siddiqsoft::WaitableQueue<MyTestObject> myContainer;

	// The worker function for each thread..
	auto workerFunction = [](std::stop_token st, siddiqsoft::WaitableQueue<MyTestObject>& myContainer)
	{
		uint64_t itemCount = 0;

		std::cout << std::this_thread::get_id() << " - Worker started." << std::endl;
		while (!st.stop_requested())
		{
			auto item = myContainer.tryWaitForNextItem();
			if (item.has_value()) itemCount++;
		}
		std::cout << std::this_thread::get_id() << " - Worker ended..." << itemCount << std::endl;
	};

	// Create the workers..
	std::array<std::jthread, THREAD_COUNT> threadPool {};
	for (int i = 0; i < THREAD_COUNT; i++)
	{
		threadPool[i] = std::jthread(workerFunction, std::ref(myContainer));
	}

	for (auto i = 0; i < ITERATION_COUNT; i++)
	{
		myContainer.push(std::move(MyTestObject {std::format("MyObject:{}:{}", i, ITERATION_COUNT)}));
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

	EXPECT_EQ(ITERATION_COUNT, myContainer.addCounter()) << myContainer.size();
}
