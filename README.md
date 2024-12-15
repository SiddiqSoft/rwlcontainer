RWLContainer : Reader-writer protected containers
-------------------------------------------

[![Build Status](https://dev.azure.com/siddiqsoft/siddiqsoft/_apis/build/status/SiddiqSoft.rwlcontainer?branchName=main)](https://dev.azure.com/siddiqsoft/siddiqsoft/_build/latest?definitionId=8&branchName=main)
![](https://img.shields.io/nuget/v/SiddiqSoft.RWLContainer)
![](https://img.shields.io/github/v/tag/SiddiqSoft/RWLContainer)
![](https://img.shields.io/azure-devops/tests/siddiqsoft/siddiqsoft/8)
![](https://img.shields.io/azure-devops/coverage/siddiqsoft/siddiqsoft/8)

# Objective

## WaitableQueue
- Implements a thread-safe `queue`
- Any number of threads can `tryWaitItem` with a given timeout on the object.
- The queue *must* be given ownership of the `StorageType` and the thread receiving the object is going to destroy the object.

## RWLContainer
- Avoid re-implementing the rw-lock; standard C++ (since C++14) has a good reader-writer lock implementation.
- Provide a simple, convenience layer for dictionary containers.
- The internal storage type is a `std::shared_ptr<>`

## Requirements
- You must be able to use [`<shared_mutex>`](https://en.cppreference.com/w/cpp/thread/shared_mutex) and [`<mutex>`](https://en.cppreference.com/w/cpp/thread/mutex).
- Minimal target is `C++20`.
- We use [`nlohmann::json`](https://github.com/nlohmann/json) only in our tests and the library is aware to provide a conversion operator if library is detected.
- CMake build system with development on MacOSX, Linux (Fedora) and Windows (VS2022).

# Usage

- Use the nuget [SiddiqSoft.RWLContainer](https://www.nuget.org/packages/SiddiqSoft.RWLContainer/)
- Use CPM to integrate into you CMake project


## Examples
```cpp
#include "gtest/gtest.h"
#include "nlohmann/json.hpp"
#include "siddiqsoft/RWLContainer.hpp"


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
```



<small align="right">

&copy; 2021 Siddiq Software LLC. All rights reserved.

</small>
