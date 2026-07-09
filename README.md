RWLContainer : Reader-writer protected containers
-------------------------------------------

[![Build Status](https://dev.azure.com/siddiqsoft/siddiqsoft/_apis/build/status/SiddiqSoft.rwlcontainer?branchName=main)](https://dev.azure.com/siddiqsoft/siddiqsoft/_build/latest?definitionId=8&branchName=main)
![](https://img.shields.io/nuget/v/SiddiqSoft.RWLContainer)
![](https://img.shields.io/github/v/tag/SiddiqSoft/RWLContainer)
![](https://img.shields.io/azure-devops/tests/siddiqsoft/siddiqsoft/8)
![](https://img.shields.io/azure-devops/coverage/siddiqsoft/siddiqsoft/8)

# Overview

This library provides convenient wrappers around C++20 standard library synchronization primitives:

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

---

# Classes and Methods

## RWLContainer

A thread-safe dictionary with reader-writer locking for efficient concurrent access.

```
RWLContainer<KeyType, StorageType>
├── Configuration
│   ├── ReplaceExisting
│   └── FailOnCollission
├── Methods
│   ├── add(key, value)
│   ├── add(key, shared_ptr)
│   ├── add(key, callback)
│   ├── remove(key)
│   ├── find(key)
│   ├── size()
│   ├── scan(callback)
│   └── toJson()
└── Properties
    ├── addCounter
    └── removeCounter
```

### RWLContainer Methods

| Method | Description | Link |
|--------|-------------|------|
| `add(key, StorageType&&)` | Add element by moving value | [Details](API.md#addconst-keytype-key-storagetype-value---storagetypeptr) |
| `add(key, StorageTypePtr&&)` | Add element via shared_ptr | [Details](API.md#addconst-keytype-key-storagetypeptr-value---storagetypeptr) |
| `add(key, callback)` | Add element via callback | [Details](API.md#addconst-keytype-key-functionstoragetypptrconst-keytype-callback---storagetypeptr) |
| `remove(key)` | Remove and return element | [Details](API.md#removeconst-keytype-key---storagetypeptr) |
| `find(key)` | Find element without removing | [Details](API.md#findconst-keytype-key---storagetypeptr) |
| `size()` | Get number of elements | [Details](API.md#size-const---size_t) |
| `scan(callback)` | Iterate and find elements | [Details](API.md#scancallback---storagetypeptr) |
| `toJson()` | Serialize to JSON | [Details](API.md#tojson---nlohmannjson-optional) |

---

## WaitableQueue

A thread-safe queue with timeout-based waiting for producer-consumer patterns.

```
WaitableQueue<StorageType>
├── Methods
│   ├── push(value)
│   ├── emplace(value)
│   ├── tryWaitItem(timeout)
│   ├── waitUntilEmpty(timeout)
│   ├── size()
│   ├── addCounter()
│   ├── removeCounter()
│   └── toJson()
└── Properties
    ├── Deleted: copy constructor
    ├── Deleted: move constructor
    ├── Deleted: copy assignment
    └── Deleted: move assignment
```

### WaitableQueue Methods

| Method | Description | Link |
|--------|-------------|------|
| `push(StorageType&&)` | Add element to queue | [Details](API.md#pushstoragetype-value---void) |
| `emplace(StorageType&&)` | Construct element in-place | [Details](API.md#emplacestoragetype-value---void) |
| `tryWaitItem(timeout)` | Wait for element with timeout | [Details](API.md#trywaititemstoragetype-timeout---stdoptionalstoragetype) |
| `waitUntilEmpty(timeout)` | Wait until queue is empty | [Details](API.md#waituntilemptytimeout---stdoptionalsize_t) |
| `size()` | Get number of elements | [Details](API.md#size---size_t) |
| `addCounter()` | Get total adds | [Details](API.md#addcounter---uint64_t) |
| `removeCounter()` | Get total removes | [Details](API.md#removecounter---uint64_t) |
| `toJson()` | Serialize to JSON | [Details](API.md#tojson---nlohmannjson-optional-1) |

---

# API Documentation

For detailed API documentation, method signatures, and comprehensive examples, see [API.md](docs/API.md) and some notes on [tests](docs/Tests.md).

# Usage

- Use the nuget [SiddiqSoft.RWLContainer](https://www.nuget.org/packages/SiddiqSoft.RWLContainer/)
- Use CPM to integrate into your CMake project

## Quick Example: RWLContainer

```cpp
#include "siddiqsoft/RWLContainer.hpp"

int main() {
    siddiqsoft::RWLContainer<std::string, int> cache;
    
    // Add items
    cache.add("count", 42);
    cache.add("value", 100);
    
    // Find items
    if (auto val = cache.find("count")) {
        std::cout << "Found: " << *val << std::endl;
    }
    
    // Remove items
    cache.remove("count");
    
    std::cout << "Size: " << cache.size() << std::endl;
    
    return 0;
}
```

## Quick Example: WaitableQueue

```cpp
#include "siddiqsoft/WaitableQueue.hpp"
#include <thread>

int main() {
    siddiqsoft::WaitableQueue<std::string> queue;
    
    // Producer thread
    std::thread producer([&queue]() {
        for (int i = 0; i < 5; ++i) {
            queue.push(std::string("item_") + std::to_string(i));
        }
    });
    
    // Consumer thread
    std::thread consumer([&queue]() {
        while (true) {
            auto item = queue.tryWaitItem(std::chrono::milliseconds(500));
            if (item) {
                std::cout << "Processed: " << *item << std::endl;
            } else {
                break;
            }
        }
    });
    
    producer.join();
    consumer.join();
    
    return 0;
}
```

---

# Versioning

Uses [GitVersion](https://gitversion.net/) for semantic versioning (`MAJOR.MINOR.PATCH`). Version is automatically determined from git history.

---

<small align="right">

&copy; 2021 Siddiq Software LLC. All rights reserved.

</small>
