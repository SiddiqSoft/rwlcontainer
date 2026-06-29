# Getting Started

A quick guide to get started with RWLContainer and WaitableQueue.

## Installation

### Option 1: NuGet (Windows)

```bash
nuget install SiddiqSoft.RWLContainer
```

### Option 2: CPM (CMake)

Add to your `CMakeLists.txt`:

```cmake
include(cmake/CPM.cmake)

CPMAddPackage("gh:SiddiqSoft/RWLContainer@1.5.0")

target_link_libraries(myapp PRIVATE SiddiqSoft::RWLContainer)
```

### Option 3: Manual

1. Clone the repository: `git clone https://github.com/SiddiqSoft/RWLContainer.git`
2. Copy `include/siddiqsoft/` to your project
3. Include the headers in your code

## Requirements

- **C++20** or later
- **CMake 3.20+** (if using CMake)
- Supported platforms: Windows, Linux, macOS

## Your First Program

### Step 1: Create a simple cache

```cpp
#include "siddiqsoft/RWLContainer.hpp"
#include <iostream>

int main() {
    // Create a thread-safe cache
    siddiqsoft::RWLContainer<std::string, int> cache;
    
    // Add items
    cache.add("count", 42);
    cache.add("value", 100);
    
    // Find items
    if (auto val = cache.find("count")) {
        std::cout << "Found: " << *val << std::endl;
    }
    
    // Check size
    std::cout << "Cache size: " << cache.size() << std::endl;
    
    return 0;
}
```

### Step 2: Compile and run

```bash
# Using g++
g++ -std=c++20 -o myapp myapp.cpp

# Using clang
clang++ -std=c++20 -o myapp myapp.cpp

# Using MSVC
cl /std:c++latest myapp.cpp
```

## Your First Queue

### Step 1: Create a producer-consumer program

```cpp
#include "siddiqsoft/WaitableQueue.hpp"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    siddiqsoft::WaitableQueue<std::string> queue;
    
    // Producer thread
    std::thread producer([&queue]() {
        for (int i = 0; i < 5; ++i) {
            queue.push(std::string("item_") + std::to_string(i));
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });
    
    // Consumer thread
    std::thread consumer([&queue]() {
        while (auto item = queue.tryWaitItem(std::chrono::milliseconds(500))) {
            std::cout << "Processed: " << *item << std::endl;
        }
    });
    
    producer.join();
    consumer.join();
    
    return 0;
}
```

### Step 2: Compile and run

```bash
g++ -std=c++20 -pthread -o myqueue myqueue.cpp
./myqueue
```

## Common Patterns

### Pattern 1: Thread-Safe Cache

```cpp
siddiqsoft::RWLContainer<std::string, int> cache;

// Multiple threads can read simultaneously
std::thread reader1([&]() {
    auto val = cache.find("key");
});

std::thread reader2([&]() {
    auto val = cache.find("key");
});

// Only one thread can write at a time
std::thread writer([&]() {
    cache.add("key", 42);
});
```

### Pattern 2: Lazy Initialization

```cpp
auto item = cache.add("key", [](const auto& key) {
    // This callback is only called if key doesn't exist
    return std::make_shared<ExpensiveObject>(key);
});
```

### Pattern 3: Producer-Consumer

```cpp
siddiqsoft::WaitableQueue<int> queue;

std::thread producer([&]() {
    for (int i = 0; i < 100; ++i) {
        queue.push(i);
    }
});

std::thread consumer([&]() {
    while (auto item = queue.tryWaitItem(std::chrono::milliseconds(100))) {
        std::cout << "Got: " << *item << std::endl;
    }
});
```

## Next Steps

1. **Read the API documentation:**
   - [RWLContainer API](RWLContainer)
   - [WaitableQueue API](WaitableQueue)

2. **Explore examples:**
   - [Examples](Examples)

3. **Learn configuration:**
   - [Configuration Guide](Configuration)

4. **Get answers:**
   - [FAQ](FAQ)

## Troubleshooting

### Compilation Error: "C++20 required"

Make sure you're using C++20 or later:

```bash
# GCC/Clang
g++ -std=c++20 myapp.cpp

# MSVC
cl /std:c++latest myapp.cpp
```

### Linking Error: "undefined reference"

Make sure you're linking against the library (if using CMake):

```cmake
target_link_libraries(myapp PRIVATE SiddiqSoft::RWLContainer)
```

### Runtime Error: "Segmentation fault"

Common causes:
1. Accessing a `shared_ptr` after it's been deleted
2. Using the queue after it's been destroyed
3. Not checking if `find()` or `tryWaitItem()` returned a valid value

Always check return values:

```cpp
if (auto val = cache.find("key")) {
    std::cout << *val << std::endl;  // Safe
}
```

## Performance Tips

1. **Use appropriate timeouts** - Balance between responsiveness and CPU usage
2. **Keep callbacks brief** - They execute within locks
3. **Monitor queue depth** - Use `size()` and counters to detect bottlenecks
4. **Use batch processing** - Process multiple items at once for efficiency

## Getting Help

- **GitHub Issues:** https://github.com/SiddiqSoft/RWLContainer/issues
- **Documentation:** See the wiki pages
- **Examples:** Check the [Examples](Examples) page

---

**Version:** 1.5.0  
**Last Updated:** 2024
