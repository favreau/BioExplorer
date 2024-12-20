/*
    Copyright 2015 - 2024 Blue Brain Project / EPFL

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#pragma once

#include "Timeout.h"

namespace core
{
/**
 * Executes the given function at most once every 'wait' milliseconds.
 *
 * Inspired by https://remysharp.com/2010/07/21/throttling-function-calls.
 */
struct Throttle
{
    using Function = std::function<void()>;
    void operator()(const Function& fn, const int64_t wait = 100);
    void operator()(const Function& fn, const Function& later, const int64_t wait = 100);

private:
    using time_point = std::chrono::time_point<std::chrono::high_resolution_clock>;
    time_point _last;
    bool _haveLast = false;
    Timeout _timeout;
    std::mutex _mutex;
};
} // namespace core
