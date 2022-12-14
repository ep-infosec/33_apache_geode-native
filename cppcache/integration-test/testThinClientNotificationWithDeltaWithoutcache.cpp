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
 * testThinClientDeltaWithNotification.cpp
 *
 *  Created on: May 18, 2009
 *      Author: pkumar
 */

#define ROOT_NAME "testThinClientNotificationWithDeltaWithoutcache"

#include "DeltaEx.hpp"
#include "fw_dunit.hpp"
#include <string>
#include "SerializationRegistry.hpp"
#include "CacheRegionHelper.hpp"
#include "CacheImpl.hpp"

using apache::geode::client::Cacheable;
using apache::geode::client::CacheableKey;
using apache::geode::client::CacheHelper;
using apache::geode::client::CacheRegionHelper;
using apache::geode::client::IllegalStateException;

CacheHelper *cacheHelper = nullptr;

#include "locator_globals.hpp"

#define CLIENT1 s1p1
#define CLIENT2 s1p2
#define SERVER1 s2p1
#include "LocatorHelper.hpp"

int DeltaEx::toDeltaCount = 0;
int DeltaEx::toDataCount = 0;
int DeltaEx::fromDeltaCount = 0;
int DeltaEx::fromDataCount = 0;
int DeltaEx::cloneCount = 0;

int PdxDeltaEx::m_toDeltaCount = 0;
int PdxDeltaEx::m_toDataCount = 0;
int PdxDeltaEx::m_fromDeltaCount = 0;
int PdxDeltaEx::m_fromDataCount = 0;
int PdxDeltaEx::m_cloneCount = 0;

void initClient(const bool isthinClient) {
  if (cacheHelper == nullptr) {
    cacheHelper = new CacheHelper(isthinClient);
  }
  ASSERT(cacheHelper, "Failed to create a CacheHelper client instance.");
}

void initClientNoPools() {
  cacheHelper = new CacheHelper(0);
  ASSERT(cacheHelper, "Failed to create a CacheHelper client instance.");
}

void cleanProc() {
  if (cacheHelper != nullptr) {
    delete cacheHelper;
    cacheHelper = nullptr;
  }
}

CacheHelper *getHelper() {
  ASSERT(cacheHelper != nullptr, "No cacheHelper initialized.");
  return cacheHelper;
}

void createPooledRegion(const std::string &name, bool ackMode,
                        const std::string &locators,
                        const std::string &poolname,
                        bool clientNotificationEnabled = false,
                        bool cachingEnable = true) {
  LOG("createRegion_Pool() entered.");
  std::cout << "Creating region --  " << name << " ackMode is " << ackMode
            << "\n"
            << std::flush;
  auto regPtr =
      getHelper()->createPooledRegion(name, ackMode, locators, poolname,
                                      cachingEnable, clientNotificationEnabled);
  ASSERT(regPtr != nullptr, "Failed to create region.");
  LOG("Pooled Region created.");
}

void createRegionCachingDisabled(const char *name, bool ackMode,
                                 bool clientNotificationEnabled = false) {
  LOG("createRegion() entered.");
  std::cout << "Creating region --  " << name << " ackMode is " << ackMode
            << "\n"
            << std::flush;
  // ack, caching
  auto regPtr = getHelper()->createRegion(name, ackMode, false, nullptr,
                                          clientNotificationEnabled);
  ASSERT(regPtr != nullptr, "Failed to create region.");
  LOG("Region created.");
}

const char *keys[] = {"Key-1", "Key-2", "Key-3", "Key-4"};

const char *regionNames[] = {"DistRegionAck", "DistRegionAck1"};

const bool USE_ACK = true;
const bool NO_ACK = false;

DUNIT_TASK_DEFINITION(CLIENT1, CreateClient1)
  {
    initClient(true);
    createPooledRegion(regionNames[0], USE_ACK, locatorsG, "__TESTPOOL1_", true,
                       false);  // without LRU
    try {
      auto serializationRegistry =
          CacheRegionHelper::getCacheImpl(cacheHelper->getCache().get())
              ->getSerializationRegistry();
      serializationRegistry->addDataSerializableType(DeltaEx::create, 1);
    } catch (IllegalStateException &) {
      //  ignore exception caused by type reregistration.
    }
  }
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(CLIENT1, CreateClient1_NoPools)
  {
    initClientNoPools();
    createRegionCachingDisabled(regionNames[0], USE_ACK, true);  // without LRU
    try {
      auto serializationRegistry =
          CacheRegionHelper::getCacheImpl(cacheHelper->getCache().get())
              ->getSerializationRegistry();
      serializationRegistry->addDataSerializableType(DeltaEx::create, 1);
    } catch (IllegalStateException &) {
      //  ignore exception caused by type reregistration.
    }
  }
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(CLIENT2, CreateClient2)
  {
    initClient(true);
    createPooledRegion(regionNames[0], USE_ACK, locatorsG, "__TESTPOOL1_", true,
                       false);
    try {
      auto serializationRegistry =
          CacheRegionHelper::getCacheImpl(cacheHelper->getCache().get())
              ->getSerializationRegistry();
      serializationRegistry->addDataSerializableType(DeltaEx::create, 1);
    } catch (IllegalStateException &) {
      //  ignore exception caused by type reregistration.
    }
    DeltaEx::fromDataCount = 0;
    DeltaEx::fromDeltaCount = 0;
  }
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(CLIENT2, CreateClient2_NoPools)
  {
    initClientNoPools();
    createRegionCachingDisabled(regionNames[0], USE_ACK, true);  // without LRU
    try {
      auto serializationRegistry =
          CacheRegionHelper::getCacheImpl(cacheHelper->getCache().get())
              ->getSerializationRegistry();
      serializationRegistry->addDataSerializableType(DeltaEx::create, 1);
    } catch (IllegalStateException &) {
      //  ignore exception caused by type reregistration.
    }
    DeltaEx::fromDataCount = 0;
    DeltaEx::fromDeltaCount = 0;
  }
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(CLIENT2, Client2_RegisterInterest)
  {
    auto regPtr = getHelper()->getRegion(regionNames[0]);
    std::vector<std::shared_ptr<CacheableKey>> vec;
    auto keyPtr = CacheableKey::create(keys[0]);
    vec.push_back(keyPtr);
    regPtr->registerKeys(vec);
  }
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(CLIENT1, Client1_Put)
  {
    auto keyPtr = CacheableKey::create(keys[0]);
    DeltaEx *ptr = new DeltaEx();
    std::shared_ptr<Cacheable> valPtr(ptr);

    auto regPtr = getHelper()->getRegion(regionNames[0]);
    regPtr->put(keyPtr, valPtr);
    ptr->setDelta(true);
    regPtr->put(keyPtr, valPtr);
  }
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(CLIENT2, Client2_VerifyDelta)
  {
    // Wait for notification
    SLEEP(1000);
    LOG(std::string("From delta count ") +
        std::to_string(DeltaEx::fromDeltaCount) + "  From data count " +
        std::to_string(DeltaEx::fromDataCount));
    // In case of Cacheless client only full object would arrive on notification
    // channe
    ASSERT(DeltaEx::fromDataCount == 2,
           "DeltaEx::fromDataCount should have been 2");
    ASSERT(DeltaEx::fromDeltaCount == 0,
           "DeltaEx::fromDeltaCount should have been 0");
  }
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(CLIENT1, CloseCache1)
  { cleanProc(); }
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(CLIENT2, CloseCache2)
  { cleanProc(); }
END_TASK_DEFINITION
DUNIT_TASK_DEFINITION(SERVER1, CloseServer1)
  {
    if (isLocalServer) {
      CacheHelper::closeServer(1);
      LOG("SERVER1 stopped");
    }
  }
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(SERVER1, CreateServer1_ForDelta)
  {
    // starting servers
    if (isLocalServer) {
      CacheHelper::initServer(1, "cacheserver_with_delta.xml", locatorsG);
    }
  }
END_TASK_DEFINITION

DUNIT_MAIN
  {
    CALL_TASK(CreateLocator1);

    CALL_TASK(CreateServer1_ForDelta);

    CALL_TASK(CreateClient1);
    CALL_TASK(CreateClient2);

    CALL_TASK(Client2_RegisterInterest);

    CALL_TASK(Client1_Put);
    CALL_TASK(Client2_VerifyDelta);

    CALL_TASK(CloseCache1);
    CALL_TASK(CloseCache2);

    CALL_TASK(CloseServer1);

    CALL_TASK(CloseLocator1);
  }
END_MAIN
