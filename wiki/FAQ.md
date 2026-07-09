# FAQ

Frequently asked questions about RWLContainer and WaitableQueue.

## General Questions

### Q: What C++ standard is required?

**A:** C++20 or later. The library uses C++20 features like concepts, `std::shared_mutex`, and `std::counting_semaphore`.

### Q: What platforms are supported?

**A:** The library is tested on:
- Windows (Visual Studio 2022)
- Linux (Fedora)
- macOS

All platforms are tested in the CI/CD pipeline.

### Q: Is the library thread-safe?

**A:** Yes, all operations are thread-safe. RWLContainer uses `std::shared_mutex` for reader-writer locking, and WaitableQueue uses `std::shared_mutex` with `std::counting_semaphore`.

### Q: Can I use these classes in production?

**A:** Yes, the library is production-ready with comprehensive test coverage and CI/CD validation.

### Q: How do I install the library?

**A:** 
- **NuGet (Windows):** `nuget install SiddiqSoft.RWLContainer`
- **CPM (CMake):** `CPMAddPackage("gh:SiddiqSoft/RWLContainer@1.5.3")`
- **Manual:** Include the header files from `include/siddiqsoft/`

---

## RWLContainer Questions

### Q: What's the difference between `ReplaceExisting` and `FailOnCollission`?

**A:** 
- `ReplaceExisting=true` - Replaces the existing value with the new one
- `FailOnCollission=true` - Returns empty `shared_ptr` on collision (takes precedence)

When both are false (default), the existing value is returned on collision.

### Q: Can I use RWLContainer with custom key types?

**A:** Yes, as long as your key type is hashable and comparable. You may need to provide a custom hash function for non-standard types.

```cpp
struct MyKey {
    int id;
    std::string name;
};

struct MyKeyHash {
    size_t operator()(const MyKey& k) const {
        return std::hash<int>()(k.id) ^ (std::hash<std::string>()(k.name) << 1);
    }
};

struct MyKeyEqual {
    bool operator()(const MyKey& a, const MyKey& b) const {
        return a.id == b.id && a.name == b.name;
    }
};

using MyContainer = siddiqsoft::RWLContainer<
    MyKey, 
    int, 
    std::unordered_map<MyKey, std::shared_ptr<int>, MyKeyHash, MyKeyEqual>
>;
```

### Q: What happens if I store pointers in RWLContainer?

**A:** Don't do this. The library is designed for value types. If you need to store pointers, wrap them in `shared_ptr` and pass them to the `add()` method.

```cpp
// ❌ BAD: Storing raw pointers
siddiqsoft::RWLContainer<std::string, int*> cache;

// ✅ GOOD: Storing values
siddiqsoft::RWLContainer<std::string, int> cache;

// ✅ GOOD: Storing complex objects
siddiqsoft::RWLContainer<std::string, MyObject> cache;
```

### Q: How do I iterate over all items in RWLContainer?

**A:** Use the `scan()` method with a callback that always returns false:

```cpp
std::vector<std::pair<std::string, std::shared_ptr<int>>> items;

cache.scan([&items](const auto& key, auto& value) {
    items.push_back({key, value});
    return false;  // Continue scanning
});
```

### Q: Can I modify values while iterating with `scan()`?

**A:** No, `scan()` uses a shared read lock. You cannot modify the container during iteration. If you need to modify, collect the keys first, then modify them separately.

### Q: What's the performance impact of using `shared_ptr`?

**A:** Minimal. `shared_ptr` has a small overhead for reference counting, but it's negligible compared to the benefits of automatic memory management and thread safety.

### Q: How do I handle exceptions in callbacks?

**A:** Exceptions in callbacks will propagate to the caller of `add()`. Wrap your callback logic in try-catch if needed:

```cpp
auto item = cache.add("key", [](const auto& key) {
    try {
        return std::make_shared<MyType>(key);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return std::shared_ptr<MyType>();
    }
});
```

### Q: Can I use RWLContainer as a singleton?

**A:** Yes, but be careful with thread safety during initialization:

```cpp
siddiqsoft::RWLContainer<std::string, int>& getCache() {
    static siddiqsoft::RWLContainer<std::string, int> cache;
    return cache;
}
```

---

## WaitableQueue Questions

### Q: Why can't I copy or move WaitableQueue?

**A:** WaitableQueue contains synchronization primitives (`std::shared_mutex`, `std::counting_semaphore`) that cannot be safely copied or moved. These primitives maintain internal state that would be corrupted if copied.

### Q: What happens if I call `tryWaitItem()` on an empty queue?

**A:** It waits for the specified timeout. If no item arrives within the timeout, it returns an empty `std::optional`.

### Q: Can I have multiple producers and consumers?

**A:** Yes, WaitableQueue is designed for multiple producers and consumers. Each `push()` signals one waiting consumer.

### Q: What's the difference between `push()` and `emplace()`?

**A:** Both add items to the queue. `emplace()` is slightly more efficient for complex types as it constructs the item in-place. For simple types, they're equivalent.

### Q: How do I know when the queue is empty?

**A:** Use `size()` to check the current size, or use `waitUntilEmpty()` to wait until the queue is empty.

```cpp
if (queue.size() == 0) {
    std::cout << "Queue is empty" << std::endl;
}

// Or wait for it to become empty
auto finalSize = queue.waitUntilEmpty(std::chrono::milliseconds(5000));
if (finalSize && *finalSize == 0) {
    std::cout << "Queue drained successfully" << std::endl;
}
```

### Q: What timeout should I use for `tryWaitItem()`?

**A:** It depends on your use case:
- **Real-time systems:** 10-50ms
- **Normal applications:** 100-500ms
- **Background tasks:** 1000ms+

Shorter timeouts are more responsive but use more CPU. Longer timeouts are more efficient but have higher latency.

### Q: Can I interrupt a waiting consumer?

**A:** Not directly. The consumer will wait for the timeout to expire. You can use an atomic flag to signal shutdown:

```cpp
std::atomic<bool> shouldStop {false};

std::thread consumer([&]() {
    while (!shouldStop) {
        if (auto item = queue.tryWaitItem(std::chrono::milliseconds(100))) {
            processItem(*item);
        }
    }
});

// Later, to stop the consumer
shouldStop = true;
consumer.join();
```

### Q: How do I handle backpressure when producers are faster than consumers?

**A:** The queue will grow. You can monitor the queue size and either:
1. Add more consumer threads
2. Reduce producer rate
3. Implement batch processing in consumers

```cpp
// Monitor queue depth
if (queue.size() > 1000) {
    std::cout << "Queue backlog detected" << std::endl;
    // Reduce producer rate or add consumers
}
```

### Q: What happens if a consumer crashes?

**A:** The queue continues to operate. Other consumers will continue processing items. You should implement error handling in your consumer threads.

### Q: Can I use WaitableQueue with non-movable types?

**A:** No, WaitableQueue requires types that satisfy the `Movable` concept (move-constructible and move-assignable). This is enforced at compile time.

### Q: How do I gracefully shutdown the queue?

**A:** Use this pattern:

```cpp
std::atomic<bool> shouldStop {false};

// Signal producers to stop
shouldStop = true;

// Wait for producers to finish
producer.join();

// Wait for queue to drain
queue.waitUntilEmpty(std::chrono::milliseconds(5000));

// Stop consumers
consumer.join();
```

---

## Performance Questions

### Q: How does RWLContainer compare to `std::unordered_map` with manual locking?

**A:** RWLContainer provides the same performance as manual locking with `std::shared_mutex`, but with less boilerplate code and fewer opportunities for bugs.

### Q: What's the overhead of using WaitableQueue vs. a raw `std::queue`?

**A:** WaitableQueue adds the overhead of:
- `std::shared_mutex` for synchronization
- `std::counting_semaphore` for signaling
- Atomic counters for diagnostics

This overhead is negligible compared to the benefits of thread-safe producer-consumer coordination.

### Q: Can I improve performance by using a custom container?

**A:** Yes, you can provide a custom container to RWLContainer:

```cpp
using MyContainer = std::map<std::string, std::shared_ptr<int>>;
siddiqsoft::RWLContainer<std::string, int, MyContainer> cache;
```

Use `std::map` for ordered access, or `std::unordered_map` (default) for faster average access.

### Q: How do I profile RWLContainer and WaitableQueue?

**A:** Use the built-in counters:

```cpp
auto startAdds = cache.toJson()["adds"];
// ... do work ...
auto endAdds = cache.toJson()["adds"];
std::cout << "Operations: " << (endAdds - startAdds) << std::endl;
```

Or use external profiling tools like perf, Instruments, or Visual Studio Profiler.

---

## Troubleshooting

### Q: I'm getting compilation errors with RWLContainer

**A:** Make sure you're using C++20 or later. Check your compiler flags:
- **GCC/Clang:** `-std=c++20`
- **MSVC:** `/std:c++latest` or `/std:c++20`

### Q: WaitableQueue is not compiling with my custom type

**A:** Your type must satisfy the `Movable` concept. Make sure it has:
- A move constructor
- A move assignment operator

```cpp
struct MyType {
    MyType(MyType&&) = default;              // Move constructor
    MyType& operator=(MyType&&) = default;   // Move assignment
};
```

### Q: I'm getting deadlocks with RWLContainer

**A:** Deadlocks can occur if:
1. You're holding a lock and trying to acquire another lock
2. Your callback in `add()` tries to access the container

Solution: Keep callbacks brief and don't access the container from within callbacks.

### Q: WaitableQueue is consuming too much CPU

**A:** Your timeout is too short. Increase it:

```cpp
// ❌ Too short - high CPU usage
auto item = queue.tryWaitItem(std::chrono::microseconds(1));

// ✅ Better - reasonable CPU usage
auto item = queue.tryWaitItem(std::chrono::milliseconds(100));
```

### Q: I'm losing items in WaitableQueue

**A:** Make sure you're:
1. Checking the return value of `tryWaitItem()`
2. Not destroying the queue while items are still being processed
3. Using appropriate timeouts

---

## Integration Questions

### Q: Can I use RWLContainer with nlohmann/json?

**A:** Yes, if you include `nlohmann/json.hpp` before including RWLContainer:

```cpp
#include "nlohmann/json.hpp"
#include "siddiqsoft/RWLContainer.hpp"

auto json = cache.toJson();
```

### Q: Can I use these classes with other libraries?

**A:** Yes, they're standard C++ classes that work with any C++20-compatible library.

### Q: How do I integrate with CMake?

**A:** Using CPM:

```cmake
CPMAddPackage("gh:SiddiqSoft/RWLContainer@1.5.3")
target_link_libraries(myapp PRIVATE SiddiqSoft::RWLContainer)
```

Or manually:

```cmake
target_include_directories(myapp PRIVATE /path/to/rwlcontainer/include)
```

---

**Version:** 1.5.3  
**Last Updated:** 2024
