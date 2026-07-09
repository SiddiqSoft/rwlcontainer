# RWLContainer API Reference

A thread-safe dictionary container with reader-writer locking for efficient concurrent access.

## Overview

`RWLContainer` is a template class that wraps `std::unordered_map` with `std::shared_mutex` to provide safe concurrent access. Multiple threads can read simultaneously, while writes are exclusive. All values are stored as `shared_ptr` for automatic memory management.

```cpp
template <class KeyType,
          class StorageType,
          typename StorageContainer = std::unordered_map<KeyType, std::shared_ptr<StorageType>>>
class RWLContainer
```

## Template Parameters

| Parameter | Description | Default |
|-----------|-------------|---------|
| `KeyType` | The key type (e.g., `std::string`, `int`) | Required |
| `StorageType` | The value type to store (use value types, not pointers) | Required |
| `StorageContainer` | The underlying container type | `std::unordered_map<KeyType, std::shared_ptr<StorageType>>` |

## Configuration Properties

### ReplaceExisting

```cpp
bool ReplaceExisting {false};
```

Controls behavior when adding a key that already exists:
- `true` - Replaces the existing value
- `false` - Returns the existing value without modification

### FailOnCollission

```cpp
bool FailOnCollission {false};
```

Controls behavior when adding a key that already exists:
- `true` - Returns empty `shared_ptr` on collision
- `false` - Returns existing value on collision

**Note:** `FailOnCollission` takes precedence over `ReplaceExisting`.

## Methods

### add(key, value)

Adds or updates an element by moving a value into a `shared_ptr`.

```cpp
StorageTypePtr add(const KeyType& key, StorageType&& value)
```

**Parameters:**
- `key` - The key to associate with the value
- `value` - The value to store (moved into shared_ptr)

**Returns:** `shared_ptr<StorageType>` to the newly inserted or existing item, or empty `shared_ptr` on collision with `FailOnCollission=true`

**Thread Safety:** Exclusive write lock

**Collision Behavior:**
- `FailOnCollission=true` → Returns empty `shared_ptr`
- `ReplaceExisting=true` → Replaces existing value
- Otherwise → Returns existing value

**Example:**
```cpp
siddiqsoft::RWLContainer<std::string, int> cache;
auto item = cache.add("key1", 42);
if (item) {
    std::cout << "Added: " << *item << std::endl;
}
```

**Throws:** `std::runtime_error` if insertion fails

---

### add(key, shared_ptr)

Adds or updates an element using an existing `shared_ptr`.

```cpp
StorageTypePtr add(const KeyType& key, StorageTypePtr&& value)
```

**Parameters:**
- `key` - The key to associate with the value
- `value` - The `shared_ptr` to store (moved into container)

**Returns:** `shared_ptr<StorageType>` to the newly inserted or existing item

**Thread Safety:** Exclusive write lock

**Note:** The input `shared_ptr` is moved; the original reference becomes invalid.

**Example:**
```cpp
auto ptr = std::make_shared<int>(42);
auto item = cache.add("key1", std::move(ptr));
```

---

### add(key, callback)

Adds an element by invoking a callback function for lazy initialization.

```cpp
StorageTypePtr add(const KeyType& key, 
                   std::function<StorageTypePtr(const KeyType&)>&& newObjectCallback)
```

**Parameters:**
- `key` - The key to add
- `newObjectCallback` - Callback that accepts the key and returns `shared_ptr<StorageType>`

**Returns:** `shared_ptr<StorageType>` to the newly created or existing item

**Thread Safety:** Exclusive write lock

**Note:** The callback executes within the write lock, so keep it brief.

**Example:**
```cpp
auto item = cache.add("key1", [](const auto& key) {
    return std::make_shared<int>(42);
});
```

---

### remove(key)

Removes and returns an element by key.

```cpp
[[nodiscard]] StorageTypePtr remove(const KeyType& key)
```

**Parameters:**
- `key` - The key to remove

**Returns:** `shared_ptr<StorageType>` to the removed item, or empty `shared_ptr` if key not found

**Thread Safety:** Exclusive write lock

**Note:** The returned `shared_ptr` keeps the item alive even after removal from container.

**Example:**
```cpp
auto removed = cache.remove("key1");
if (removed) {
    std::cout << "Removed: " << *removed << std::endl;
}
```

---

### find(key)

Finds and returns an element by key without removing it.

```cpp
StorageTypePtr find(const KeyType& key)
```

**Parameters:**
- `key` - The key to find

**Returns:** `shared_ptr<StorageType>` to the found item, or empty `shared_ptr` if key not found

**Thread Safety:** Shared read lock (allows concurrent reads)

**Note:** Multiple threads can call `find()` simultaneously.

**Example:**
```cpp
auto item = cache.find("key1");
if (item) {
    std::cout << "Found: " << *item << std::endl;
}
```

---

### size()

Returns the number of elements in the container (const-qualified).

```cpp
size_t size() const
```

**Returns:** The number of elements currently in the container

**Thread Safety:** Shared read lock

**Complexity:** O(1)

**Example:**
```cpp
std::cout << "Container has " << cache.size() << " items" << std::endl;
```

---

### scan(callback)

Iterates through all elements and returns the first match.

```cpp
StorageTypePtr scan(std::function<bool(const KeyType&, StorageTypePtr&)> scanCallback)
```

**Parameters:**
- `scanCallback` - Callback function that receives `(key, value)` and returns `true` to stop iteration

**Returns:** `shared_ptr<StorageType>` to the first matching item, or empty `shared_ptr` if no match found

**Thread Safety:** Shared read lock (entire scan is atomic)

**Note:** The callback is invoked for each element until it returns `true`.

**Example:**
```cpp
auto item = cache.scan([](const auto& key, auto& value) {
    return key.find("target") != std::string::npos;
});
```

---

### toJson()

Serializes container metadata to JSON (const-qualified, returns const value).

```cpp
auto toJson() const -> nlohmann::json const
```

**Returns:** JSON object with container statistics

**Thread Safety:** Shared read lock | **Signature:** `auto toJson() const -> nlohmann::json const`

**Requirements:** `nlohmann/json.hpp` must be included before this header

**JSON Structure:**
```json
{
  "_typver": "RWLContainer/1.5.5",
  "adds": 42,
  "removes": 10,
  "ReplaceExisting": false,
  "FailOnCollission": false,
  "size": 32
}
```

**Example:**
```cpp
#include "nlohmann/json.hpp"
auto json = cache.toJson();
std::cout << json.dump(2) << std::endl;
```

---

## Thread Safety

All operations in `RWLContainer` are thread-safe:

- **Read operations** (`find`, `size`, `scan`) use shared locks - multiple threads can execute concurrently
- **Write operations** (`add`, `remove`) use exclusive locks - only one thread can execute at a time
- **Atomic counters** (`_counterAdds`, `_counterRemoves`) are thread-safe

## Performance Characteristics

| Operation | Complexity | Lock Type |
|-----------|-----------|-----------|
| `add()` | O(1) average | Exclusive |
| `remove()` | O(1) average | Exclusive |
| `find()` | O(1) average | Shared |
| `size()` | O(1) | Shared |
| `scan()` | O(n) | Shared |

## Best Practices

1. **Use appropriate key types** - Prefer `std::string` or numeric types with good hash functions
2. **Keep values small** - Store large objects by reference using `shared_ptr`
3. **Minimize callback duration** - Callbacks in `add()` execute within the write lock
4. **Check return values** - Always check if `find()` or `remove()` returned a valid `shared_ptr`
5. **Configure collision behavior** - Set `ReplaceExisting` and `FailOnCollission` based on your use case

## Common Patterns

### Cache Pattern

```cpp
siddiqsoft::RWLContainer<std::string, std::string> cache;

// Try to find, if not found, add
auto item = cache.find("key");
if (!item) {
    item = cache.add("key", "value");
}
```

### Lazy Initialization

```cpp
auto item = cache.add("key", [](const auto& key) {
    return std::make_shared<ExpensiveObject>(key);
});
```

### Atomic Replace

```cpp
cache.ReplaceExisting = true;
auto newItem = cache.add("key", "new_value");
```

---

**Version:** 1.5.5  
**Last Updated:** 2024
