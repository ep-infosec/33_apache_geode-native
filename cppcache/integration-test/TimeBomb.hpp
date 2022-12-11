/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#ifndef GEODE_INTEGRATION_TEST_TIMEBOMB_H_
#define GEODE_INTEGRATION_TEST_TIMEBOMB_H_

#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

class TimeBomb {
 public:
  explicit TimeBomb(const std::chrono::milliseconds& sleep,
                    std::function<void()> cleanup);

  ~TimeBomb() noexcept;

  void arm();
  void disarm();

  void run();

 protected:
  bool enabled_;
  std::mutex mutex_;
  std::condition_variable cv_;

  std::thread thread_;
  std::function<void()> callback_;
  std::chrono::milliseconds sleep_;
};

#endif  // GEODE_INTEGRATION_TEST_TIMEBOMB_H_
