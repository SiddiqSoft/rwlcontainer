# Examples

Real-world usage examples for RWLContainer and WaitableQueue.

## RWLContainer Examples

### Example 1: Simple Cache

A basic cache for storing computed values.

```cpp
#include "siddiqsoft/RWLContainer.hpp"
#include <iostream>
#include <string>

int main() {
    siddiqsoft::RWLContainer<std::string, int> cache;
    
    // Add items
    cache.add("count", 42);
    cache.add("value", 100);
    cache.add("status", 200);
    
    // Find items
    if (auto val = cache.find("count")) {
        std::cout << "Count: " << *val << std::endl;
    }
    
    // Update items
    cache.ReplaceExisting = true;
    cache.add("count", 50);
    
    // Remove items
    if (auto removed = cache.remove("status")) {
        std::cout << "Removed: " << *removed << std::endl;
    }
    
    // Check size
    std::cout << "Cache size: " << cache.size() << std::endl;
    
    return 0;
}
```

### Example 2: Lazy Initialization Cache

A cache that lazily initializes expensive objects.

```cpp
#include "siddiqsoft/RWLContainer.hpp"
#include <iostream>
#include <string>
#include <memory>

class ExpensiveResource {
public:
    ExpensiveResource(const std::string& id) : _id(id) {
        std::cout << "Creating resource: " << id << std::endl;
    }
    
    const std::string& getId() const { return _id; }
    
private:
    std::string _id;
};

int main() {
    siddiqsoft::RWLContainer<std::string, ExpensiveResource> resourceCache;
    
    // First access - creates the resource
    auto res1 = resourceCache.add("resource1", [](const auto& key) {
        return std::make_shared<ExpensiveResource>(key);
    });
    
    // Second access - returns cached resource
    auto res2 = resourceCache.add("resource1", [](const auto& key) {
        return std::make_shared<ExpensiveResource>(key);
    });
    
    std::cout << "Same resource: " << (res1.get() == res2.get()) << std::endl;
    
    return 0;
}
```

### Example 3: Thread-Safe Registry

A registry that enforces unique keys.

```cpp
#include "siddiqsoft/RWLContainer.hpp"
#include <iostream>
#include <thread>
#include <vector>

int main() {
    siddiqsoft::RWLContainer<std::string, int> registry;
    registry.FailOnCollission = true;
    
    std::vector<std::thread> threads;
    
    // Multiple threads trying to register
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([&registry, i]() {
            auto result = registry.add("unique_key", i);
            if (result) {
                std::cout << "Thread " << i << " registered successfully" << std::endl;
            } else {
                std::cout << "Thread " << i << " failed - key already exists" << std::endl;
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    return 0;
}
```

### Example 4: Scan and Filter

Finding items using custom predicates.

```cpp
#include "siddiqsoft/RWLContainer.hpp"
#include <iostream>
#include <string>

int main() {
    siddiqsoft::RWLContainer<std::string, int> data;
    
    // Add some data
    data.add("apple", 5);
    data.add("banana", 3);
    data.add("cherry", 8);
    data.add("date", 2);
    
    // Find first item with value > 5
    auto item = data.scan([](const auto& key, auto& value) {
        return *value > 5;
    });
    
    if (item) {
        std::cout << "Found item with value > 5: " << *item << std::endl;
    }
    
    // Find first key starting with 'b'
    auto item2 = data.scan([](const auto& key, auto& value) {
        return key[0] == 'b';
    });
    
    if (item2) {
        std::cout << "Found item starting with 'b': " << *item2 << std::endl;
    }
    
    return 0;
}
```

### Example 5: JSON Serialization

Exporting container statistics to JSON.

```cpp
#include "siddiqsoft/RWLContainer.hpp"
#include "nlohmann/json.hpp"
#include <iostream>

int main() {
    siddiqsoft::RWLContainer<std::string, int> cache;
    
    // Add and remove items
    cache.add("key1", 10);
    cache.add("key2", 20);
    cache.add("key3", 30);
    cache.remove("key1");
    
    // Export to JSON
    auto json = cache.toJson();
    std::cout << json.dump(2) << std::endl;
    
    // Output:
    // {
    //   "_typver": "RWLContainer/1.5.5",
    //   "adds": 3,
    //   "removes": 1,
    //   "ReplaceExisting": false,
    //   "FailOnCollission": false,
    //   "size": 2
    // }
    
    return 0;
}
```

## WaitableQueue Examples

### Example 1: Basic Producer-Consumer

A simple producer-consumer pattern.

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

### Example 2: Multiple Consumers

Multiple consumer threads processing items from a single queue.

```cpp
#include "siddiqsoft/WaitableQueue.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>

int main() {
    siddiqsoft::WaitableQueue<int> queue;
    
    // Producer thread
    std::thread producer([&queue]() {
        for (int i = 0; i < 20; ++i) {
            queue.push(i);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });
    
    // Multiple consumer threads
    std::vector<std::thread> consumers;
    for (int i = 0; i < 3; ++i) {
        consumers.emplace_back([&queue, i]() {
            while (auto item = queue.tryWaitItem(std::chrono::milliseconds(500))) {
                std::cout << "Consumer " << i << " processed: " << *item << std::endl;
            }
        });
    }
    
    producer.join();
    for (auto& t : consumers) {
        t.join();
    }
    
    return 0;
}
```

### Example 3: Graceful Shutdown

Properly shutting down producer-consumer threads.

```cpp
#include "siddiqsoft/WaitableQueue.hpp"
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>

int main() {
    siddiqsoft::WaitableQueue<std::string> queue;
    std::atomic<bool> shouldStop {false};
    
    // Producer thread
    std::thread producer([&queue, &shouldStop]() {
        int count = 0;
        while (!shouldStop) {
            queue.push(std::string("item_") + std::to_string(count++));
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        std::cout << "Producer stopped" << std::endl;
    });
    
    // Consumer thread
    std::thread consumer([&queue]() {
        while (auto item = queue.tryWaitItem(std::chrono::milliseconds(500))) {
            std::cout << "Processed: " << *item << std::endl;
        }
        std::cout << "Consumer stopped" << std::endl;
    });
    
    // Let them run for a bit
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Graceful shutdown
    std::cout << "Initiating shutdown..." << std::endl;
    shouldStop = true;
    producer.join();
    
    // Wait for queue to drain
    auto finalSize = queue.waitUntilEmpty(std::chrono::milliseconds(5000));
    if (finalSize && *finalSize == 0) {
        std::cout << "Queue drained successfully" << std::endl;
    } else {
        std::cout << "Queue drain timeout with " << *finalSize << " items remaining" << std::endl;
    }
    
    consumer.join();
    
    return 0;
}
```

### Example 4: Batch Processing

Processing items in batches for efficiency.

```cpp
#include "siddiqsoft/WaitableQueue.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>

int main() {
    siddiqsoft::WaitableQueue<int> queue;
    
    // Producer thread
    std::thread producer([&queue]() {
        for (int i = 0; i < 100; ++i) {
            queue.push(i);
        }
    });
    
    // Consumer thread with batch processing
    std::thread consumer([&queue]() {
        std::vector<int> batch;
        const size_t batchSize = 10;
        
        while (auto item = queue.tryWaitItem(std::chrono::milliseconds(100))) {
            batch.push_back(*item);
            
            if (batch.size() >= batchSize) {
                // Process batch
                int sum = 0;
                for (int val : batch) {
                    sum += val;
                }
                std::cout << "Batch sum: " << sum << std::endl;
                batch.clear();
            }
        }
        
        // Process remaining items
        if (!batch.empty()) {
            int sum = 0;
            for (int val : batch) {
                sum += val;
            }
            std::cout << "Final batch sum: " << sum << std::endl;
        }
    });
    
    producer.join();
    consumer.join();
    
    return 0;
}
```

### Example 5: Throughput Monitoring

Monitoring queue throughput and performance.

```cpp
#include "siddiqsoft/WaitableQueue.hpp"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    siddiqsoft::WaitableQueue<int> queue;
    
    // Producer thread
    std::thread producer([&queue]() {
        for (int i = 0; i < 10000; ++i) {
            queue.push(i);
        }
    });
    
    // Consumer thread
    std::thread consumer([&queue]() {
        while (auto item = queue.tryWaitItem(std::chrono::milliseconds(100))) {
            // Simulate processing
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    });
    
    // Monitoring thread
    std::thread monitor([&queue]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        for (int i = 0; i < 5; ++i) {
            auto startAdds = queue.addCounter();
            auto startRemoves = queue.removeCounter();
            auto startSize = queue.size();
            
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            auto endAdds = queue.addCounter();
            auto endRemoves = queue.removeCounter();
            auto endSize = queue.size();
            
            std::cout << "Throughput:" << std::endl;
            std::cout << "  Adds/sec: " << (endAdds - startAdds) << std::endl;
            std::cout << "  Removes/sec: " << (endRemoves - startRemoves) << std::endl;
            std::cout << "  Queue size: " << endSize << std::endl;
            std::cout << "  Backlog: " << (endAdds - endRemoves) << std::endl;
        }
    });
    
    producer.join();
    consumer.join();
    monitor.join();
    
    return 0;
}
```

### Example 6: JSON Serialization

Exporting queue statistics to JSON.

```cpp
#include "siddiqsoft/WaitableQueue.hpp"
#include "nlohmann/json.hpp"
#include <iostream>
#include <thread>

int main() {
    siddiqsoft::WaitableQueue<std::string> queue;
    
    // Add some items
    std::thread producer([&queue]() {
        for (int i = 0; i < 10; ++i) {
            queue.push(std::string("item_") + std::to_string(i));
        }
    });
    
    // Remove some items
    std::thread consumer([&queue]() {
        for (int i = 0; i < 7; ++i) {
            queue.tryWaitItem(std::chrono::milliseconds(100));
        }
    });
    
    producer.join();
    consumer.join();
    
    // Export to JSON
    auto json = queue.toJson();
    std::cout << json.dump(2) << std::endl;
    
    // Output:
    // {
    //   "_typver": "WaitableQueue/1.5.5",
    //   "adds": 10,
    //   "removes": 7,
    //   "size": 3
    // }
    
    return 0;
}
```

## Advanced Examples

### Example: Distributed Task Processing

A system that distributes tasks across multiple worker threads.

```cpp
#include "siddiqsoft/WaitableQueue.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <functional>

struct Task {
    int id;
    std::function<void()> work;
};

int main() {
    siddiqsoft::WaitableQueue<Task> taskQueue;
    std::atomic<int> completedTasks {0};
    
    // Worker threads
    std::vector<std::thread> workers;
    for (int i = 0; i < 4; ++i) {
        workers.emplace_back([&taskQueue, &completedTasks, i]() {
            while (auto task = taskQueue.tryWaitItem(std::chrono::milliseconds(500))) {
                std::cout << "Worker " << i << " executing task " << task->id << std::endl;
                task->work();
                completedTasks++;
            }
        });
    }
    
    // Task producer
    std::thread producer([&taskQueue]() {
        for (int i = 0; i < 20; ++i) {
            Task task {
                i,
                [i]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            };
            taskQueue.push(std::move(task));
        }
    });
    
    producer.join();
    
    // Wait for all tasks to complete
    taskQueue.waitUntilEmpty(std::chrono::milliseconds(10000));
    
    for (auto& w : workers) {
        w.join();
    }
    
    std::cout << "Completed tasks: " << completedTasks << std::endl;
    
    return 0;
}
```

---

**Version:** 1.5.5  
**Last Updated:** 2024
