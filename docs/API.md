# RWLContainer API Documentation

## Overview

RWLContainer provides two thread-safe container classes for C++20:
- **RWLContainer**: A reader-writer lock protected dictionary/map
- **WaitableQueue**: A thread-safe queue with timeout-based waiting

Both classes use `std::shared_mutex` for efficient concurrent access with multiple readers and exclusive writers.

---

## RWLContainer

A generic dictionary container with reader-writer locking. Internally stores elements as `std::shared_ptr<StorageType>`.

### Template Parameters

```cpp
template <class KeyType,
          class StorageType,
          typename StorageContainer = std::unordered_map<KeyType, std::shared_ptr<StorageType>>>
class RWLContainer
```

- **KeyType**: The key type (e.g., `std::string`)
- **StorageType**: The value type to store (avoid pointers; use value types)
- **StorageContainer**: Underlying container (defaults to `std::unordered_map`)

### Configuration Properties

```cpp
bool ReplaceExisting {false};    // Replace existing items on add
bool FailOnCollission {false};   // Fail if key already exists
```

### Methods

#### `add(const KeyType& key, StorageType&& value) -> StorageTypePtr`

Adds an element by moving it into a `shared_ptr`.

**Parameters:**
- `key`: The key to associate with the value
- `value`: The value to store (moved into shared_ptr)

**Returns:** `std::shared_ptr<StorageType>` - The newly inserted or existing item

**Behavior:**
- If key exists and `FailOnCollission=true`: returns empty shared_ptr
- If key exists and `ReplaceExisting=false`: returns existing item
- If key exists and `ReplaceExisting=true`: replaces and returns new item
- If key doesn't exist: inserts and returns new item

**Example:**
```cpp
siddiqsoft::RWLContainer<std::string, std::string> container;
auto item = container.add("key1", std::string("value1"));
```

---

#### `add(const KeyType& key, StorageTypePtr&& value) -> StorageTypePtr`

Adds an element by moving an existing `shared_ptr`.

**Parameters:**
- `key`: The key to associate with the value
- `value`: A `shared_ptr<StorageType>` to store (moved into container)

**Returns:** `std::shared_ptr<StorageType>` - The newly inserted or existing item

**Example:**
```cpp
auto ptr = std::make_shared<std::string>("value");
container.add("key1", std::move(ptr));
```

---

#### `add(const KeyType& key, std::function<StorageTypePtr(const KeyType&)>&& callback) -> StorageTypePtr`

Adds an element via callback if the key doesn't exist. Useful for lazy initialization.

**Parameters:**
- `key`: The key to associate with the value
- `callback`: Function that creates and returns a `shared_ptr<StorageType>` given the key

**Returns:** `std::shared_ptr<StorageType>` - The newly created or existing item

**Note:** The callback executes within the write lock, so keep it brief.

**Example:**
```cpp
auto item = container.add("key1", [](const auto& key) {
    return std::make_shared<std::string>("lazy_" + key);
});
```

---

#### `remove(const KeyType& key) -> StorageTypePtr`

Removes and returns an element by key.

**Parameters:**
- `key`: The key to remove

**Returns:** `std::shared_ptr<StorageType>` - The removed item, or empty shared_ptr if not found

**Example:**
```cpp
auto removed = container.remove("key1");
if (removed) {
    // Item was found and removed
}
```

---

#### `find(const KeyType& key) -> StorageTypePtr`

Retrieves an element by key without removing it.

**Parameters:**
- `key`: The key to find

**Returns:** `std::shared_ptr<StorageType>` - The found item, or empty shared_ptr if not found

**Note:** Uses a read lock, allowing concurrent reads.

**Example:**
```cpp
auto item = container.find("key1");
if (item) {
    // Use item
}
```

---

#### `size() const -> size_t`

Returns the number of elements in the container.

**Returns:** `size_t` - Current number of items

**Note:** Uses a read lock.

**Example:**
```cpp
auto count = container.size();
```

---

#### `scan(std::function<bool(const KeyType&, StorageTypePtr&)> callback) -> StorageTypePtr`

Iterates through all elements, calling the callback for each. Returns the first item where callback returns `true`.

**Parameters:**
- `callback`: Function that receives key and value, returns `true` to stop iteration

**Returns:** `std::shared_ptr<StorageType>` - The first matching item, or empty shared_ptr if none match

**Note:** Uses a read lock for the entire scan.

**Example:**
```cpp
auto found = container.scan([](const auto& key, auto& value) {
    return key.find("target") != std::string::npos;
});
```

---

#### `toJson() -> nlohmann::json` (Optional)

Serializes container metadata to JSON (only available if `nlohmann/json.hpp` is included).

**Returns:** JSON object with:
- `_typver`: Type and version string
- `adds`: Total number of add operations
- `removes`: Total number of remove operations
- `ReplaceExisting`: Current setting
- `FailOnCollission`: Current setting
- `size`: Current number of items

**Example:**
```cpp
#include "nlohmann/json.hpp"
auto json = container.toJson();
std::cout << json.dump(2) << std::endl;
```

---

## WaitableQueue

A thread-safe queue with timeout-based waiting. Designed for producer-consumer patterns.

### Template Parameters

```cpp
template <class StorageType, class StorageContainer = std::queue<StorageType>>
    requires Movable<StorageType>
class WaitableQueue
```

- **StorageType**: The element type (must be move-constructible and move-assignable)
- **StorageContainer**: Underlying container (defaults to `std::queue`)

### Constraints

- **Non-copyable**: Cannot be copied or moved
- **Thread-safe**: All operations are protected by `std::shared_mutex`
- **Signaling**: Uses `std::counting_semaphore` for efficient waiting

### Methods

#### `push(StorageType&& value) -> void`

Adds an element to the end of the queue and signals waiting threads.

**Parameters:**
- `value`: The element to add (moved into queue)

**Example:**
```cpp
siddiqsoft::WaitableQueue<std::string> queue;
queue.push(std::string("item1"));
queue.push(std::move(myString));
```

---

#### `emplace(StorageType&& value) -> void`

Constructs an element in-place at the end of the queue and signals waiting threads.

**Parameters:**
- `value`: The element to emplace (moved into queue)

**Example:**
```cpp
queue.emplace(std::string("item1"));
```

---

#### `tryWaitItem(std::chrono::milliseconds timeout = 100ms) -> std::optional<StorageType>`

Waits for an item to become available with a timeout. Returns immediately if an item is available.

**Parameters:**
- `timeout`: Maximum time to wait (defaults to 100ms)

**Returns:** `std::optional<StorageType>` - The dequeued item, or empty if timeout occurs

**Note:** 
- Uses a semaphore for efficient waiting
- Multiple threads can wait concurrently
- Item is removed from queue upon successful retrieval

**Example:**
```cpp
while (true) {
    auto item = queue.tryWaitItem(std::chrono::milliseconds(500));
    if (item) {
        // Process item
        std::cout << *item << std::endl;
    } else {
        // Timeout occurred
        break;
    }
}
```

---

#### `waitUntilEmpty(std::chrono::milliseconds timeout = 1500ms) -> bool`

Spins until the queue is empty or timeout occurs. Useful for graceful shutdown.

**Parameters:**
- `timeout`: Maximum time to wait (defaults to 1500ms)

**Returns:** `bool` - `true` if queue became empty, `false` if timeout occurred

**Note:** Use this before destroying the queue to ensure all items are processed.

**Example:**
```cpp
// Signal producers to stop
// Wait for queue to drain
auto result = queue.waitUntilEmpty(std::chrono::milliseconds(2000));
if (result) {
    // Queue is empty
}
```

---

#### `size() -> size_t`

Returns the current number of elements in the queue.

**Returns:** `size_t` - Current queue size

**Note:** Uses a read lock.

**Example:**
```cpp
auto count = queue.size();
```

---

#### `addCounter() -> uint64_t`

Returns the total number of items added to the queue.

**Returns:** `uint64_t` - Cumulative add count

**Example:**
```cpp
auto totalAdded = queue.addCounter();
```

---

#### `removeCounter() -> uint64_t`

Returns the total number of items successfully retrieved via `tryWaitItem`.

**Returns:** `uint64_t` - Cumulative remove count

**Example:**
```cpp
auto totalRemoved = queue.removeCounter();
```

---

#### `toJson() -> nlohmann::json` (Optional)

Serializes queue metadata to JSON (only available if `nlohmann/json.hpp` is included).

**Returns:** JSON object with:
- `_typver`: Type and version string
- `adds`: Total number of add operations
- `removes`: Total number of remove operations
- `size`: Current number of items

**Example:**
```cpp
#include "nlohmann/json.hpp"
auto json = queue.toJson();
std::cout << json.dump(2) << std::endl;
```

---

## Usage Examples

### RWLContainer Example

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
    
    // Scan for items
    cache.scan([](const auto& key, auto& value) {
        std::cout << key << " = " << *value << std::endl;
        return false; // Continue scanning
    });
    
    // Remove items
    cache.remove("count");
    
    std::cout << "Size: " << cache.size() << std::endl;
    
    return 0;
}
```

### WaitableQueue Example

```cpp
#include "siddiqsoft/WaitableQueue.hpp"
#include <thread>

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
        while (true) {
            auto item = queue.tryWaitItem(std::chrono::milliseconds(500));
            if (item) {
                std::cout << "Processed: " << *item << std::endl;
            } else {
                break; // Timeout, no more items
            }
        }
    });
    
    producer.join();
    consumer.join();
    
    // Ensure queue is empty
    queue.waitUntilEmpty();
    
    return 0;
}
```

---

## Thread Safety

Both containers are fully thread-safe:

- **RWLContainer**: Multiple threads can read concurrently; writes are exclusive
- **WaitableQueue**: All operations are protected; multiple consumers can wait concurrently

No external synchronization is required when using these containers.

---

## Performance Considerations

- **RWLContainer**: Use for scenarios with many readers and few writers
- **WaitableQueue**: Efficient for producer-consumer patterns with semaphore-based signaling
- Both use `std::shared_ptr` for automatic memory management
- Read operations use `std::shared_lock` for concurrent access

---

## Namespace

All classes are in the `siddiqsoft` namespace:

```cpp
using namespace siddiqsoft;
// or
siddiqsoft::RWLContainer<...>
siddiqsoft::WaitableQueue<...>
```
