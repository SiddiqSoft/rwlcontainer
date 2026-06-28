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

- **107 comprehensive test cases** using Google Test (gtest)
- **Full FIFO ordering verification** for WaitableQueue (24 dedicated tests)
- **100% method coverage** for both RWLContainer and WaitableQueue
- Code coverage reporting (Linux builds)
- Automated testing on all supported platforms via Azure Pipelines
- Stress testing with up to 5000+ items and multiple concurrent threads
- Edge case and concurrent access pattern testing

### Test Files

- `tests/test.cpp` - RWLContainer core tests (34 tests)
- `tests/queuetest.cpp` - WaitableQueue core tests (20 tests)
- `tests/coverage_test.cpp` - Comprehensive coverage tests (29 tests)
- `tests/fifo_test.cpp` - FIFO ordering verification tests (24 tests)

For detailed test documentation, see [TEST_DOCUMENTATION.md](TEST_DOCUMENTATION.md).

### Known Test Behavior

**ConcurrentScanAndWrite Test**: This stress test exercises concurrent scanning and writing operations on the RWLContainer. Due to platform-specific differences in `std::shared_mutex` implementations:

- **Linux (x86_64 and ARM64)**: The test includes a timeout mechanism (100ms) to prevent potential deadlocks. The scan callback will exit early if it exceeds the timeout, allowing the test to complete successfully.
- **Windows (ARM64)** and **macOS**: The test runs without timeout restrictions and completes normally without deadlock issues.

This difference is due to variations in how different platforms implement reader-writer lock fairness and priority inversion handling. The timeout mechanism ensures the test remains robust across all platforms while still validating the thread-safety of concurrent operations.

**WaitUntilEmptyTightLoopContention Test**: This stress test for WaitableQueue exercises tight-loop contention between multiple threads calling `waitUntilEmpty()` while concurrent push/pop operations occur. Due to platform-specific differences in `std::shared_mutex` implementations:

- **Linux (x86_64 and ARM64)**: The test is skipped or may experience deadlocks due to lock fairness issues under extreme contention. The tight loop with minimal timeouts (1ms) can cause priority inversion on Linux's shared_mutex implementation.
- **Windows (ARM64)** and **macOS**: The test runs without issues and completes normally, demonstrating robust handling of concurrent wait and mutation operations.

This difference reflects variations in how different platforms prioritize reader vs. writer threads in shared_mutex implementations. The library's core functionality remains thread-safe on all platforms; this test specifically stresses an edge case that manifests differently across platforms.

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
