# WaitableQueue API Reference

A thread-safe queue with timeout-based waiting for producer-consumer patterns.

## Overview

`WaitableQueue` is a template class that implements a high-performance queue designed for multi-threaded producer-consumer scenarios. It combines `std::deque` with `std::shared_mutex` and `std::counting_semaphore` for efficient concurrent access.

```cpp
template <class StorageType>
    requires Movable<StorageType>
class WaitableQueue
```

## Template Parameters

| Parameter | Description | Constraints |
|-----------|-------------|-------------|
| `StorageType` | The type of items to store in the queue | Must be move-constructible and move-assignable |

## Key Characteristics

- **FIFO Ordering** - Items are retrieved in the order they were added
- **Non-Copyable** - Cannot be copied due to synchronization primitives
- **Non-Movable** - Cannot be moved due to synchronization primitives
- **Efficient Waiting** - Uses `std::counting_semaphore` for efficient producer-consumer signaling
- **Timeout Support** - All wait operations support configurable timeouts

## Methods

### push(value)

Adds an item to the end of the queue and signals waiting consumers.

```cpp
void push(StorageType&& value)
```

**Parameters:**
- `value` - The item to add (moved into queue)

**Returns:** `void`

**Thread Safety:** Exclusive write lock

**Note:** The input value is moved; the original reference becomes invalid.

**Example:**
```cpp
siddiqsoft::WaitableQueue<std::string> queue;
queue.push("hello");
queue.push(std::move(myString));
```

---

### emplace(value)

Constructs and adds an item to the end of the queue in-place.

```cpp
void emplace(StorageType&& value)
```

**Parameters:**
- `value` - The item to construct and add (moved into queue)

**Returns:** `void`

**Thread Safety:** Exclusive write lock

**Note:** More efficient than `push()` when you want to construct the item in-place.

**Example:**
```cpp
queue.emplace("hello");
queue.emplace(MyType{42, "value"});
```

---

### tryWaitItem(timeout)

Waits for an item with timeout and returns it if available.

```cpp
[[nodiscard]] std::optional<StorageType> tryWaitItem(
    std::chrono::milliseconds timeoutDuration = std::chrono::milliseconds(100))
```

**Parameters:**
- `timeoutDuration` - Maximum time to wait (default: 100ms)

**Returns:** `std::optional<StorageType>` containing the dequeued item, or empty if timeout

**Thread Safety:** Exclusive write lock for item removal

**FIFO Ordering:** Returns items in the order they were added

**Note:** Multiple consumers can wait concurrently; only one gets each item.

**Example:**
```cpp
while (true) {
    auto item = queue.tryWaitItem(std::chrono::milliseconds(500));
    if (item) {
        std::cout << "Got: " << *item << std::endl;
    } else {
        std::cout << "Timeout" << std::endl;
        break;
    }
}
```

---

### waitUntilEmpty(timeout)

Waits until the queue is empty or timeout occurs.

```cpp
std::optional<size_t> waitUntilEmpty(
    std::chrono::milliseconds timeoutDuration = std::chrono::milliseconds(1500))
```

**Parameters:**
- `timeoutDuration` - Maximum time to wait (default: 1500ms)

**Returns:** `std::optional<size_t>` containing the final queue size (0 if empty, >0 if timeout)

**Thread Safety:** Shared read lock for size checks

**Note:** Spins with increasing sleep intervals (32ms, 64ms, 96ms, ...) until queue is empty or timeout.

**Use Case:** Graceful shutdown scenarios where you want to ensure all items are processed.

**Example:**
```cpp
// Signal producers to stop
// Wait for queue to drain
auto finalSize = queue.waitUntilEmpty(std::chrono::milliseconds(2000));
if (finalSize && *finalSize == 0) {
    std::cout << "Queue is empty" << std::endl;
} else {
    std::cout << "Timeout with " << *finalSize << " items remaining" << std::endl;
}
```

---

### size()

Returns the current number of items in the queue (const-qualified).

```cpp
size_t size() const
```

**Returns:** The number of items currently in the queue

**Thread Safety:** Shared read lock

**Complexity:** O(1)

**Note:** Size can change immediately after return due to concurrent operations.

**Example:**
```cpp
std::cout << "Queue has " << queue.size() << " items" << std::endl;
```

---

### addCounter()

Returns the total number of items added to the queue (const-qualified, returns const value).

```cpp
auto addCounter() const -> uint64_t const
```

**Returns:** The cumulative count of all items added via `push()` or `emplace()`

**Thread Safety:** Atomic operation

**Note:** This counter never decreases and provides a measure of total throughput.

**Signature:** `auto addCounter() const -> uint64_t const`

**Example:**
```cpp
std::cout << "Total added: " << queue.addCounter() << std::endl;
```

---

### removeCounter()

Returns the total number of items successfully retrieved from the queue (const-qualified, returns const value).

```cpp
auto removeCounter() const -> uint64_t const
```

**Returns:** The cumulative count of all items successfully retrieved via `tryWaitItem()`

**Thread Safety:** Atomic operation

**Note:** This counter never decreases. Should eventually equal `addCounter()` when queue is fully drained.

**Signature:** `auto removeCounter() const -> uint64_t const`

**Example:**
```cpp
std::cout << "Total removed: " << queue.removeCounter() << std::endl;
```

---

### toJson()

Serializes queue metadata to JSON (const-qualified).

```cpp
nlohmann::json toJson() const
```

**Returns:** JSON object with queue statistics

**Thread Safety:** Atomic operations and shared read lock | **Signature:** `nlohmann::json toJson() const`

**Requirements:** `nlohmann/json.hpp` must be included before this header

**JSON Structure:**
```json
{
  "_typver": "WaitableQueue/1.5.3",
  "adds": 100,
  "removes": 95,
  "size": 5
}
```

**Example:**
```cpp
#include "nlohmann/json.hpp"
auto json = queue.toJson();
std::cout << json.dump(2) << std::endl;
```

---

## Deleted Operations

The following operations are explicitly deleted to prevent misuse:

```cpp
WaitableQueue(const WaitableQueue&) = delete;           // Copy constructor
WaitableQueue& operator=(const WaitableQueue&) = delete; // Copy assignment
WaitableQueue(WaitableQueue&&) = delete;                 // Move constructor
WaitableQueue& operator=(WaitableQueue&&) = delete;      // Move assignment
```

**Reason:** Queue contains synchronization primitives (`std::shared_mutex`, `std::counting_semaphore`) that cannot be safely copied or moved.

---

## Thread Safety

All operations in `WaitableQueue` are thread-safe:

- **Producer operations** (`push`, `emplace`) use exclusive locks
- **Consumer operations** (`tryWaitItem`) use exclusive locks for item removal
- **Query operations** (`size`, `addCounter`, `removeCounter`) use atomic operations or shared locks
- **Semaphore signaling** ensures efficient producer-consumer coordination

## Performance Characteristics

| Operation | Complexity | Lock Type |
|-----------|-----------|-----------|
| `push()` | O(1) | Exclusive |
| `emplace()` | O(1) | Exclusive |
| `tryWaitItem()` | O(1) | Exclusive |
| `size()` | O(1) | Shared |
| `addCounter()` | O(1) | Atomic |
| `removeCounter()` | O(1) | Atomic |

## Best Practices

1. **Use appropriate timeouts** - Balance between responsiveness and CPU usage
2. **Handle empty optionals** - Always check if `tryWaitItem()` returned a value
3. **Graceful shutdown** - Use `waitUntilEmpty()` before destroying the queue
4. **Monitor counters** - Use `addCounter()` and `removeCounter()` for diagnostics
5. **Avoid blocking callbacks** - Keep producer/consumer logic outside the queue

## Common Patterns

### Basic Producer-Consumer

```cpp
siddiqsoft::WaitableQueue<std::string> queue;

// Producer thread
std::thread producer([&]() {
    for (int i = 0; i < 10; ++i) {
        queue.push(std::string("item_") + std::to_string(i));
    }
});

// Consumer thread
std::thread consumer([&]() {
    while (auto item = queue.tryWaitItem(std::chrono::milliseconds(500))) {
        std::cout << "Processed: " << *item << std::endl;
    }
});

producer.join();
consumer.join();
```

### Multiple Consumers

```cpp
std::vector<std::thread> consumers;
for (int i = 0; i < 4; ++i) {
    consumers.emplace_back([&queue]() {
        while (auto item = queue.tryWaitItem(std::chrono::milliseconds(500))) {
            // Process item
        }
    });
}

// Add items from main thread
for (int i = 0; i < 100; ++i) {
    queue.push(i);
}

// Wait for all consumers to finish
for (auto& t : consumers) {
    t.join();
}
```

### Graceful Shutdown

```cpp
std::atomic<bool> shouldStop {false};

std::thread producer([&]() {
    while (!shouldStop) {
        queue.push("item");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
});

std::thread consumer([&]() {
    while (auto item = queue.tryWaitItem(std::chrono::milliseconds(500))) {
        // Process item
    }
});

// Signal shutdown
shouldStop = true;
producer.join();

// Wait for queue to drain
queue.waitUntilEmpty(std::chrono::milliseconds(5000));
consumer.join();
```

### Throughput Monitoring

```cpp
auto startAdds = queue.addCounter();
auto startRemoves = queue.removeCounter();

std::this_thread::sleep_for(std::chrono::seconds(1));

auto endAdds = queue.addCounter();
auto endRemoves = queue.removeCounter();

std::cout << "Adds/sec: " << (endAdds - startAdds) << std::endl;
std::cout << "Removes/sec: " << (endRemoves - startRemoves) << std::endl;
std::cout << "Pending: " << queue.size() << std::endl;
```

---

**Version:** 1.5.3  
**Last Updated:** 2024
