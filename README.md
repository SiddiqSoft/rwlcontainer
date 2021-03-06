RWLContainer : Reader-writer protected container
-------------------------------------------

[![CodeQL](https://github.com/SiddiqSoft/RWLContainer/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/SiddiqSoft/RWLContainer/actions/workflows/codeql-analysis.yml)
[![Build Status](https://dev.azure.com/siddiqsoft/siddiqsoft/_apis/build/status/SiddiqSoft.rwlcontainer?branchName=main)](https://dev.azure.com/siddiqsoft/siddiqsoft/_build/latest?definitionId=8&branchName=main)
![](https://img.shields.io/nuget/v/SiddiqSoft.RWLContainer)
![](https://img.shields.io/github/v/tag/SiddiqSoft/RWLContainer)
![](https://img.shields.io/azure-devops/tests/siddiqsoft/siddiqsoft/8)
![](https://img.shields.io/azure-devops/coverage/siddiqsoft/siddiqsoft/8)

# Objective
- Avoid re-implementing the rw-lock; standard C++ (since C++14) has a good reader-writer lock implementation.
- Provide a simple, convenience layer for dictionary containers.
- The storage type is a `std::shared_ptr<>`


# Requirements
- You must be able to use [`<shared_mutex>`](https://en.cppreference.com/w/cpp/thread/shared_mutex) and [`<mutex>`](https://en.cppreference.com/w/cpp/thread/mutex).
- Minimal target is `C++17`.
- The build and tests are for Visual Studio 2019 under x64.
- We use [`nlohmann::json`](https://github.com/nlohmann/json) only in our tests and the library is aware to provide a conversion operator if library is detected.

# Usage

- Use the nuget [SiddiqSoft.RWLContainer](https://www.nuget.org/packages/SiddiqSoft.RWLContainer/)
- Copy paste..whatever works.


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

Additional [examples](tests/examples.cpp).


<small align="right">

&copy; 2021 Siddiq Software LLC. All rights reserved.

</small>
