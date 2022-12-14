#pragma once

#ifndef GEODE_INTEGRATION_TEST_THINCLIENTDURABLERECONNECT_H_
#define GEODE_INTEGRATION_TEST_THINCLIENTDURABLERECONNECT_H_

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
/*
 * ThinClientDurableReconnect.hpp
 *
 *  Created on: Nov 3, 2008
 *      Author: abhaware
 */

#include "fw_dunit.hpp"
#include "ThinClientHelper.hpp"

/* This is to test
1- Durable Client always reconnect to secondary to avoid data loss.
*/

#define CLIENT1 s1p1
#define CLIENT2 s1p2
#define SERVER1 s2p1
#define FEEDER s2p2

namespace {  // NOLINT(google-build-namespaces)

using apache::geode::client::EntryEvent;

class OperMonitor : public CacheListener {
  bool m_first, m_second, m_close;

 public:
  OperMonitor() : m_first(false), m_second(false), m_close(false) {}

  void validate() {
    LOG("validate called");
    ASSERT(m_first, "m_first event not recieved");
    ASSERT(m_close && m_second, "m_second event not recieved");
  }

  void afterCreate(const EntryEvent&) override {
    if (!m_close) {
      m_first = true;
      LOG("First Event Recieved");
    } else {
      m_second = true;
      LOG("Duplicate Recieved");
    }
  }
  void close(Region&) override {
    m_close = true;
    LOG("Listener Close Called");
  }
};

void setCacheListener(const char* regName,
                      std::shared_ptr<OperMonitor> monitor) {
  auto reg = getHelper()->getRegion(regName);
  auto attrMutator = reg->getAttributesMutator();
  attrMutator->setCacheListener(monitor);
}
std::shared_ptr<OperMonitor> mon1 = nullptr;

const char* mixKeys[] = {"D-Key-1"};

#include "ThinClientDurableInit.hpp"
#include "ThinClientTasks_C2S2.hpp"

void initClientCache(int redundancy, std::shared_ptr<OperMonitor>& mon) {
  initClientAndRegion(redundancy, 0, std::chrono::seconds(60000),
                      std::chrono::seconds(1), std::chrono::seconds(300));

  if (mon == nullptr) {
    mon = std::make_shared<OperMonitor>();
  }

  setCacheListener(regionNames[0], mon);

  getHelper()->cachePtr->readyForEvents();
  auto regPtr0 = getHelper()->getRegion(regionNames[0]);
  regPtr0->registerAllKeys(true);
}

DUNIT_TASK_DEFINITION(CLIENT1, ClientInit)
  { initClientCache(1, mon1); }
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(FEEDER, FeederInit)
  {
    initClient(true);

    createRegion(regionNames[0], USE_ACK, true);
    LOG("FeederInit complete.");

    createIntEntry(regionNames[0], mixKeys[0], 1);

    LOG("FeederUpdate complete.");
  }
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(CLIENT1, ClientDown)
  {
    getHelper()->disconnect(true);
    cleanProc();
    LOG("Clnt1Down complete: Keepalive = True");
  }
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(CLIENT1, ClientReInit)
  { initClientCache(1, mon1); }
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(CLIENT1, Verify)
  {
    LOG("Client Verify");
    mon1->validate();
  }
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(FEEDER, CloseFeeder)
  {
    cleanProc();
    LOG("FEEDER closed");
  }
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(CLIENT1, CloseClient)
  {
    mon1 = nullptr;
    cleanProc();
    LOG("CLIENT1 closed");
  }
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(SERVER1, CloseServers)
  {
    CacheHelper::closeServer(1);
    CacheHelper::closeServer(2);
    LOG("SERVERs closed");
  }
END_TASK_DEFINITION

void doThinClientDurableReconnect() {
  CALL_TASK(StartLocator);

  startServers();

  CALL_TASK(ClientInit);
  CALL_TASK(FeederInit);
  CALL_TASK(ClientDown);
  CALL_TASK(ClientReInit);
  CALL_TASK(Verify);
  CALL_TASK(CloseFeeder);
  CALL_TASK(CloseClient);
  CALL_TASK(CloseServers);

  closeLocator();
}

}  // namespace

#endif  // GEODE_INTEGRATION_TEST_THINCLIENTDURABLERECONNECT_H_
