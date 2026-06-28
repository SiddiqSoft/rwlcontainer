# Configuration Guide

This guide covers configuration options and best practices for RWLContainer and WaitableQueue.

## RWLContainer Configuration

### Collision Handling

RWLContainer provides two configuration flags to control behavior when adding a key that already exists:

#### ReplaceExisting

```cpp
bool ReplaceExisting {false};
```

**When `false` (default):**
- Existing values are preserved
- `add()` returns the existing value
- Useful for cache patterns where you want to keep the first value

**When `true`:**
- Existing values are replaced with new ones
- `add()` returns the newly added value
- Useful for update patterns where you want the latest value

**Example:**
```cpp
siddiqsoft::RWLContainer<std::string, int> cache;

// Default behavior: preserve existing
cache.add("key", 10);
auto val1 = cache.add("key", 20);  // Returns 10

// Replace behavior
cache.ReplaceExisting = true;
auto val2 = cache.add("key", 30);  // Returns 30
```

#### FailOnCollission

```cpp
bool FailOnCollission {false};
```

**When `false` (default):**
- Collisions are handled gracefully
- Returns existing value or empty `shared_ptr` based on `ReplaceExisting`
- Useful for most use cases

**When `true`:**
- Returns empty `shared_ptr` on collision
- Useful for strict key uniqueness requirements
- Allows detecting duplicate key attempts

**Note:** `FailOnCollission` takes precedence over `ReplaceExisting`.

**Example:**
```cpp
cache.FailOnCollission = true;
auto val1 = cache.add("key", 10);  // Returns shared_ptr to 10
auto val2 = cache.add("key", 20);  // Returns empty shared_ptr (collision)
```

### Configuration Patterns

#### Pattern 1: Cache with Lazy Initialization

```cpp
siddiqsoft::RWLContainer<std::string, ExpensiveObject> cache;
cache.ReplaceExisting = false;
cache.FailOnCollission = false;

// Try to find, if not found, add
auto obj = cache.find("key");
if (!obj) {
    obj = cache.add("key", [](const auto& key) {
        return std::make_shared<ExpensiveObject>(key);
    });
}
```

#### Pattern 2: Strict Key Uniqueness

```cpp
siddiqsoft::RWLContainer<std::string, int> registry;
registry.FailOnCollission = true;

auto result = registry.add("unique_key", 42);
if (!result) {
    std::cerr << "Key already exists!" << std::endl;
}
```

#### Pattern 3: Atomic Update

```cpp
siddiqsoft::RWLContainer<std::string, std::string> config;
config.ReplaceExisting = true;

// Always get the latest value
auto newValue = config.add("setting", "new_value");
```

## WaitableQueue Configuration

WaitableQueue doesn't have explicit configuration flags, but behavior can be controlled through timeout values and usage patterns.

### Timeout Selection

Choose timeouts based on your use case:

| Timeout | Use Case | Example |
|---------|----------|---------|
| 10-50ms | High-frequency processing | Real-time systems |
| 100-500ms | Normal processing | Most applications |
| 1000ms+ | Low-frequency or batch processing | Background tasks |

**Example:**
```cpp
// High-frequency consumer
while (auto item = queue.tryWaitItem(std::chrono::milliseconds(10))) {
    processItem(*item);
}

// Normal consumer
while (auto item = queue.tryWaitItem(std::chrono::milliseconds(100))) {
    processItem(*item);
}

// Low-frequency consumer
while (auto item = queue.tryWaitItem(std::chrono::milliseconds(1000))) {
    processItem(*item);
}
```

### Graceful Shutdown Configuration

```cpp
// Configure for graceful shutdown
std::atomic<bool> shouldStop {false};

std::thread producer([&]() {
    while (!shouldStop) {
        queue.push(generateItem());
    }
});

std::thread consumer([&]() {
    while (auto item = queue.tryWaitItem(std::chrono::milliseconds(500))) {
        processItem(*item);
    }
});

// Shutdown sequence
shouldStop = true;
producer.join();

// Wait for queue to drain (max 5 seconds)
auto finalSize = queue.waitUntilEmpty(std::chrono::milliseconds(5000));
if (finalSize && *finalSize > 0) {
    std::cerr << "Warning: " << *finalSize << " items not processed" << std::endl;
}

consumer.join();
```

## Performance Tuning

### RWLContainer Tuning

1. **Key Type Selection**
   - Use `std::string` for string keys (good hash function)
   - Use numeric types for numeric keys
   - Avoid complex types as keys

2. **Value Type Selection**
   - Store small values directly
   - Store large objects via `shared_ptr`
   - Avoid unnecessary copies

3. **Callback Optimization**
   - Keep callbacks in `add()` brief
   - Avoid I/O operations in callbacks
   - Pre-compute values before calling `add()`

**Example:**
```cpp
// Good: Brief callback
auto obj = cache.add("key", [](const auto& k) {
    return std::make_shared<int>(42);
});

// Bad: Long-running callback
auto obj = cache.add("key", [](const auto& k) {
    auto result = expensiveNetworkCall();  // Blocks all writers!
    return std::make_shared<Result>(result);
});
```

### WaitableQueue Tuning

1. **Timeout Selection**
   - Balance between responsiveness and CPU usage
   - Shorter timeouts = higher CPU usage
   - Longer timeouts = higher latency

2. **Batch Processing**
   - Process multiple items in a batch
   - Reduces context switching overhead

3. **Thread Count**
   - Match consumer threads to CPU cores
   - Monitor queue depth to detect bottlenecks

**Example:**
```cpp
// Batch processing
std::vector<std::string> batch;
while (auto item = queue.tryWaitItem(std::chrono::milliseconds(100))) {
    batch.push_back(*item);
    if (batch.size() >= 10) {
        processBatch(batch);
        batch.clear();
    }
}
if (!batch.empty()) {
    processBatch(batch);
}
```

## Monitoring and Diagnostics

### RWLContainer Diagnostics

```cpp
// Get container statistics
auto json = cache.toJson();
std::cout << "Container stats:" << std::endl;
std::cout << "  Size: " << json["size"] << std::endl;
std::cout << "  Total adds: " << json["adds"] << std::endl;
std::cout << "  Total removes: " << json["removes"] << std::endl;
```

### WaitableQueue Diagnostics

```cpp
// Monitor queue throughput
auto startAdds = queue.addCounter();
auto startRemoves = queue.removeCounter();

std::this_thread::sleep_for(std::chrono::seconds(1));

auto endAdds = queue.addCounter();
auto endRemoves = queue.removeCounter();

std::cout << "Queue stats:" << std::endl;
std::cout << "  Adds/sec: " << (endAdds - startAdds) << std::endl;
std::cout << "  Removes/sec: " << (endRemoves - startRemoves) << std::endl;
std::cout << "  Pending: " << queue.size() << std::endl;
std::cout << "  Backlog: " << (endAdds - endRemoves) << std::endl;
```

## Thread Safety Considerations

### RWLContainer Thread Safety

- **Read operations** are concurrent (multiple threads can read simultaneously)
- **Write operations** are exclusive (only one thread can write at a time)
- **Callbacks** execute within the write lock (keep them brief)

**Implication:** If you have many writers, consider partitioning the data across multiple containers.

### WaitableQueue Thread Safety

- **Multiple producers** can push concurrently
- **Multiple consumers** can wait concurrently (but only one gets each item)
- **Semaphore signaling** ensures efficient coordination

**Implication:** WaitableQueue scales well with multiple producers and consumers.

## Common Configuration Mistakes

### Mistake 1: Long-Running Callbacks

```cpp
// ❌ BAD: Blocks all writers
cache.add("key", [](const auto& k) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return std::make_shared<int>(42);
});

// ✅ GOOD: Pre-compute outside the container
auto value = expensiveComputation();
cache.add("key", std::move(value));
```

### Mistake 2: Ignoring Return Values

```cpp
// ❌ BAD: Doesn't check for collision
cache.FailOnCollission = true;
cache.add("key", 42);

// ✅ GOOD: Check for collision
if (auto result = cache.add("key", 42)) {
    std::cout << "Added successfully" << std::endl;
} else {
    std::cout << "Key already exists" << std::endl;
}
```

### Mistake 3: Inappropriate Timeout

```cpp
// ❌ BAD: Too short, high CPU usage
while (auto item = queue.tryWaitItem(std::chrono::microseconds(1))) {
    processItem(*item);
}

// ✅ GOOD: Appropriate timeout
while (auto item = queue.tryWaitItem(std::chrono::milliseconds(100))) {
    processItem(*item);
}
```

---

**Version:** 1.5.0  
**Last Updated:** 2024
