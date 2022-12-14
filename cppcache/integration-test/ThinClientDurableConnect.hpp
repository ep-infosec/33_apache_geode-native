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

#ifndef GEODE_INTEGRATION_TEST_THINCLIENTDURABLECONNECT_H_
#define GEODE_INTEGRATION_TEST_THINCLIENTDURABLECONNECT_H_

#include "fw_dunit.hpp"
#include "ThinClientHelper.hpp"
#include "CacheImplHelper.hpp"
#include "testUtils.hpp"

/* Test case covered:
With Four Servers
1-  ( R = 1 ) All Server Up, Check if client reconnect to either of server
having queue.
2-  (  R = 1 ) One Redundant Server down, Check if client reconnect to another
server having queue.
3-  (  R = 1 ) Both Redundant Server down, Check if client reconnect to both non
redundant server and all events are lost.
4-  (  R = 1 ) Both Non-Redundant Server down, Check if client reconnect to both
redundant server.
5-  ( R = 1 ) All Server Up, Check if client reconnect to either of server after
timeout period and all the events are lost.
*/

#define CLIENT s1p1
#define SERVER_SET1 s1p2
#define SERVER_SET2 s2p1
#define SERVER_SET3 s2p2
#define SERVER1 s1p2

bool isLocalServerList = false;
const std::string endPointsList =
    CacheHelper::getTcrEndpoints(isLocalServerList, 4);
const char* durableId = "DurableId";

#include "ThinClientDurableInit.hpp"
#include "ThinClientTasks_C2S2.hpp"

std::string getServerEndPoint(int instance) {
  int port;
  if (instance == 1) {
    port = CacheHelper::staticHostPort1;
  } else if (instance == 2) {
    port = CacheHelper::staticHostPort2;
  } else if (instance == 3) {
    port = CacheHelper::staticHostPort3;
  } else {
    port = CacheHelper::staticHostPort4;
  }

  return std::to_string(port);
}

DUNIT_TASK_DEFINITION(SERVER_SET1, S1Up)
  {
    if (isLocalServer) {
      CacheHelper::initServer(1, "cacheserver_notify_subscription.xml",
                              locatorsG);
    }
    LOG("SERVER 1 started");
  }
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(SERVER_SET1, S2Up)
  {
    if (isLocalServer) {
      CacheHelper::initServer(2, "cacheserver_notify_subscription2.xml",
                              locatorsG);
    }
    LOG("SERVER 2 started");
  }
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(SERVER_SET2, S3Up)
  {
    if (isLocalServer) {
      CacheHelper::initServer(3, "cacheserver_notify_subscription3.xml",
                              locatorsG);
    }
    LOG("SERVER 3 started");
  }
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(SERVER_SET2, S4Up)
  {
    if (isLocalServer) {
      CacheHelper::initServer(4, "cacheserver_notify_subscription4.xml",
                              locatorsG);
    }
    LOG("SERVER 4 started");
  }
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(SERVER_SET1, S1Down)
  {
    CacheHelper::closeServer(1);
    LOG("SERVER 1 closed");
  }
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(SERVER_SET1, S2Down)
  {
    CacheHelper::closeServer(2);
    LOG("SERVER 2 closed");
  }
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(SERVER_SET2, S3Down)
  {
    CacheHelper::closeServer(3);
    LOG("SERVER 3 closed");
  }
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(SERVER_SET2, S4Down)
  {
    CacheHelper::closeServer(4);
    LOG("SERVER 4 closed");
  }
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(CLIENT, ClntUp)
  {
    initClientAndRegion(1, 0);
    getHelper()->cachePtr->readyForEvents();

    auto regPtr0 = getHelper()->getRegion(regionNames[0]);
    regPtr0->registerAllKeys(true);

    LOG("Clnt1Init complete.");
  }
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(CLIENT, ClntUpNonHA)
  {
    initClientAndRegion(0, 0);
    getHelper()->cachePtr->readyForEvents();

    auto regPtr0 = getHelper()->getRegion(regionNames[0]);
    regPtr0->registerAllKeys(true);

    LOG("Clnt1Init complete.");
  }
END_TASK_DEFINITION

/* Close Client 1 with option keep alive = true*/
DUNIT_TASK_DEFINITION(CLIENT, ClntDown)
  {
    getHelper()->disconnect(true);
    cleanProc();
    LOG("Client Down complete: Keepalive = True");
  }
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(CLIENT, ClntSleep)
  {
    LOG(" Sleeping for 100 seconds");
    SLEEP(100000); /* 100 seconds for timeout*/
    LOG(" Finished sleep of 100 seconds");
  }
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(CLIENT, VerifyNonHA)
  {
    /*Test case 4 , 1 */
    LOG("Verify 1: Verify that Client has queue on S1");

    ASSERT(TestUtils::getCacheImpl(getHelper()->cachePtr)
               ->getEndpointStatus(getServerEndPoint(1)),
           "Server 1 should have queue");
    ASSERT(!TestUtils::getCacheImpl(getHelper()->cachePtr)
                ->getEndpointStatus(getServerEndPoint(2)),
           "Server 2 should not have queue");
    ASSERT(!TestUtils::getCacheImpl(getHelper()->cachePtr)
                ->getEndpointStatus(getServerEndPoint(3)),
           "Server 3 should not have queue/connection");
    ASSERT(!TestUtils::getCacheImpl(getHelper()->cachePtr)
                ->getEndpointStatus(getServerEndPoint(4)),
           "Server 4 should not have queue/connection");
  }
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(CLIENT, Verify1)
  {
    /*Test case 4 , 1 */
    LOG("Verify 1: Verify that Client has queues on S1 and S2");

    ASSERT(TestUtils::getCacheImpl(getHelper()->cachePtr)
               ->getEndpointStatus(getServerEndPoint(1)),
           "Server 1 should have queue");
    ASSERT(TestUtils::getCacheImpl(getHelper()->cachePtr)
               ->getEndpointStatus(getServerEndPoint(2)),
           "Server 2 should have queue");
    ASSERT(!TestUtils::getCacheImpl(getHelper()->cachePtr)
                ->getEndpointStatus(getServerEndPoint(3)),
           "Server 3 should not have queue/connection");
    ASSERT(!TestUtils::getCacheImpl(getHelper()->cachePtr)
                ->getEndpointStatus(getServerEndPoint(4)),
           "Server 4 should not have queue/connection");
  }
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(CLIENT, Verify2)
  {
    /*Test case 2*/
    LOG("Verify 2: Verify that Client has queues on S2 and either S3 or S4 and "
        "not both");

    ASSERT(!TestUtils::getCacheImpl(getHelper()->cachePtr)
                ->getEndpointStatus(getServerEndPoint(1)),
           "Server 1 should not have queue");
    ASSERT(TestUtils::getCacheImpl(getHelper()->cachePtr)
               ->getEndpointStatus(getServerEndPoint(2)),
           "Server 2 should have queue");
    bool server3_hasQueue = TestUtils::getCacheImpl(getHelper()->cachePtr)
                                ->getEndpointStatus(getServerEndPoint(3));
    bool server4_hasQueue = TestUtils::getCacheImpl(getHelper()->cachePtr)
                                ->getEndpointStatus(getServerEndPoint(4));
    ASSERT(((server3_hasQueue && !server4_hasQueue) ||
            (!server3_hasQueue && server4_hasQueue)),
           "Either Server 3 or Server 4 should have queue");
  }
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(CLIENT, Verify3)
  {
    /*Test case 3*/
    LOG("Verify 3: Verify that Client has queues on S3 and S4");

    ASSERT(!TestUtils::getCacheImpl(getHelper()->cachePtr)
                ->getEndpointStatus(getServerEndPoint(1)),
           "Server 1 should not have queue/connection");
    ASSERT(!TestUtils::getCacheImpl(getHelper()->cachePtr)
                ->getEndpointStatus(getServerEndPoint(2)),
           "Server 2 should not have queue/connection");
    ASSERT(TestUtils::getCacheImpl(getHelper()->cachePtr)
               ->getEndpointStatus(getServerEndPoint(3)),
           "Server 3 should have queue/connection");
    ASSERT(TestUtils::getCacheImpl(getHelper()->cachePtr)
               ->getEndpointStatus(getServerEndPoint(4)),
           "Server 4 should have queue/connection");
  }
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(CLIENT, Verify4)
  {
    /*Test case 5*/
    LOG("Verify 4: Verify that no servers have queues");

    ASSERT(!TestUtils::getCacheImpl(getHelper()->cachePtr)
                ->getEndpointStatus(getServerEndPoint(1)),
           "Server 1 should not have queue/connection");
    ASSERT(!TestUtils::getCacheImpl(getHelper()->cachePtr)
                ->getEndpointStatus(getServerEndPoint(2)),
           "Server 2 should not have queue/connection");
    ASSERT(!TestUtils::getCacheImpl(getHelper()->cachePtr)
                ->getEndpointStatus(getServerEndPoint(3)),
           "Server 3 should not have queue/connection");
    ASSERT(!TestUtils::getCacheImpl(getHelper()->cachePtr)
                ->getEndpointStatus(getServerEndPoint(4)),
           "Server 4 should not have queue/connection");
  }
END_TASK_DEFINITION

void doThinClientDurableConnect() {
  CALL_TASK(StartLocator);

  CALL_TASK(S1Up);
  CALL_TASK(ClntUpNonHA);
  CALL_TASK(S2Up);
  CALL_TASK(ClntDown);
  CALL_TASK(ClntUp);
  CALL_TASK(VerifyNonHA);
  CALL_TASK(ClntDown);
  CALL_TASK(S1Down);
  /* Presently for 4 servers only */
  /* Test case 4 */
  CALL_TASK(S1Up);
  CALL_TASK(ClntUp);
  /* Test case 1 */
  CALL_TASK(S3Up);
  CALL_TASK(S4Up);
  CALL_TASK(ClntDown);
  CALL_TASK(ClntUp);
  CALL_TASK(Verify1);
  /* Test case 2 */
  CALL_TASK(ClntDown);
  CALL_TASK(S1Down);
  CALL_TASK(ClntUp);
  CALL_TASK(ClntDown);
  CALL_TASK(ClntUp);
  CALL_TASK(Verify2);
  /* Test case 3 */
  CALL_TASK(ClntDown);
  CALL_TASK(S2Down);
  CALL_TASK(ClntUp);
  CALL_TASK(ClntDown);
  CALL_TASK(ClntUp);
  CALL_TASK(Verify3);
  CALL_TASK(ClntDown);
  CALL_TASK(ClntSleep);
  CALL_TASK(ClntUp);
  CALL_TASK(Verify4);
  /* Close Everything */
  CALL_TASK(ClntDown);
  CALL_TASK(S3Down);
  CALL_TASK(S4Down);

  CALL_TASK(CloseLocator);
}

#endif  // GEODE_INTEGRATION_TEST_THINCLIENTDURABLECONNECT_H_
