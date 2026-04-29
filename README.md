RWLContainer : Reader-writer protected containers
-------------------------------------------

[![Build Status](https://dev.azure.com/siddiqsoft/siddiqsoft/_apis/build/status/SiddiqSoft.rwlcontainer?branchName=main)](https://dev.azure.com/siddiqsoft/siddiqsoft/_build/latest?definitionId=8&branchName=main)
![](https://img.shields.io/nuget/v/SiddiqSoft.RWLContainer)
![](https://img.shields.io/github/v/tag/SiddiqSoft/RWLContainer)
![](https://img.shields.io/azure-devops/tests/siddiqsoft/siddiqsoft/8)
![](https://img.shields.io/azure-devops/coverage/siddiqsoft/siddiqsoft/8)

# Overview

This library provides convenient, production-ready wrappers around C++20 standard library synchronization primitives:

- **RWLContainer**: A reader-writer lock protected dictionary that wraps `std::unordered_map` with `std::shared_mutex` for safe concurrent access
- **WaitableQueue**: A thread-safe queue that augments `std::queue` with `std::shared_mutex` and `std::counting_semaphore` for efficient producer-consumer patterns

These classes eliminate boilerplate synchronization code and provide a simple, type-safe API for common concurrent programming scenarios.

## Requirements

- C++20 or later
- [`<shared_mutex>`](https://en.cppreference.com/w/cpp/thread/shared_mutex) and [`<mutex>`](https://en.cppreference.com/w/cpp/thread/mutex) support
- CMake 3.20+

## Platform Support

Tested and built on:
- **Windows** (Visual Studio 2022)
- **Linux** (Fedora)
- **macOS**

All platforms are tested in the CI/CD pipeline with comprehensive test coverage.

## Testing

- Unit tests using Google Test (gtest)
- Code coverage reporting (Linux builds)
- Automated testing on all supported platforms via Azure Pipelines
- Stress testing included in test suite

## Versioning

Uses [GitVersion](https://gitversion.net/) for semantic versioning (`MAJOR.MINOR.PATCH`). Version is automatically determined from git history.

# API Documentation

For detailed API documentation, method signatures, and comprehensive examples, see [API.md](API.md).

# Usage

- Use the nuget [SiddiqSoft.RWLContainer](https://www.nuget.org/packages/SiddiqSoft.RWLContainer/)
- Use CPM to integrate into your CMake project

## Quick Example

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
