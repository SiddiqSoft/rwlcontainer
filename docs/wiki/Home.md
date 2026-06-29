# RWLContainer Wiki

Welcome to the RWLContainer documentation. This library provides production-ready, thread-safe container classes for C++20 applications.

## Overview

RWLContainer offers two main classes for concurrent programming:

- **[RWLContainer](RWLContainer)** - A thread-safe dictionary with reader-writer locking
- **[WaitableQueue](WaitableQueue)** - A thread-safe queue for producer-consumer patterns

## Quick Start

### Installation

```bash
# Using NuGet (Windows)
nuget install SiddiqSoft.RWLContainer

# Using CPM (CMake)
CPMAddPackage("gh:SiddiqSoft/RWLContainer@1.5.3")
```

### Basic Usage

#### RWLContainer Example

```cpp
#include "siddiqsoft/RWLContainer.hpp"

int main() {
    siddiqsoft::RWLContainer<std::string, int> cache;
    
    // Add items
    cache.add("key1", 42);
    
    // Find items
    if (auto value = cache.find("key1")) {
        std::cout << "Value: " << *value << std::endl;
    }
    
    // Remove items
    cache.remove("key1");
    
    return 0;
}
```

#### WaitableQueue Example

```cpp
#include "siddiqsoft/WaitableQueue.hpp"

int main() {
    siddiqsoft::WaitableQueue<std::string> queue;
    
    // Producer
    std::thread producer([&]() {
        queue.push("item1");
        queue.push("item2");
    });
    
    // Consumer
    std::thread consumer([&]() {
        while (auto item = queue.tryWaitItem(std::chrono::milliseconds(500))) {
            std::cout << "Got: " << *item << std::endl;
        }
    });
    
    producer.join();
    consumer.join();
    
    return 0;
}
```

## Documentation

- **[RWLContainer API](RWLContainer)** - Complete reference for the dictionary class
- **[WaitableQueue API](WaitableQueue)** - Complete reference for the queue class
- **[Configuration](Configuration)** - Configuration options and best practices
- **[Examples](Examples)** - Real-world usage examples
- **[FAQ](FAQ)** - Frequently asked questions

## Key Features

✅ **Thread-Safe** - All operations protected by synchronization primitives  
✅ **High Performance** - Reader-writer locking allows concurrent reads  
✅ **Type-Safe** - Full C++20 template support  
✅ **Production-Ready** - Comprehensive test coverage and CI/CD  
✅ **Zero-Copy** - Move semantics and shared_ptr for efficient memory management  
✅ **JSON Support** - Optional JSON serialization with nlohmann/json  

## Requirements

- C++20 or later
- CMake 3.20+
- Supported platforms: Windows, Linux, macOS

## License

BSD 3-Clause License - See LICENSE file for details

---

**Version:** 1.5.3  
**Last Updated:** 2024  
**Maintainer:** Siddiq Software LLC
