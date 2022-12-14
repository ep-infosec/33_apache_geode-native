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
#include <sstream>

#include "fw_dunit.hpp"
#include "ThinClientHelper.hpp"

/* Testing Parameters              Param's Value
Server notify-by-subscription:                   true

Descripton:  This is to test the receiveValues flag
in register interest APIs. Client events
delivered should include creates/updates or not include them (converted into
invalidates).
*/

#define CLIENT_NBS_TRUE s1p1
/*
#define CLIENT_NBS_FALSE s1p2
#define CLIENT_NBS_DEFAULT s2p1
*/
#define SERVER_AND_FEEDER s2p2
#define SERVER1 s2p2  // duplicate definition required for a helper file

using apache::geode::client::EntryEvent;
using apache::geode::client::Exception;

class EventListener : public CacheListener {
 public:
  int m_creates;
  int m_updates;
  int m_invalidates;
  int m_destroys;
  std::string m_name;

  void check(const EntryEvent &event, const char *eventType) {
    try {
      auto keyPtr = std::dynamic_pointer_cast<CacheableString>(event.getKey());
      auto valuePtr =
          std::dynamic_pointer_cast<CacheableInt32>(event.getNewValue());
      std::stringstream strm;
      strm << m_name << ": " << eventType << ": Key = " << keyPtr->value()
           << (valuePtr ? valuePtr->toString() : "nullptr");
      LOG(strm.str());
    } catch (const Exception &excp) {
      std::stringstream strm;
      strm << m_name << ": " << eventType << ": " << excp.getName() << ": "
           << excp.what();
      LOG(strm.str());
    } catch (...) {
      std::stringstream strm;
      strm << m_name << ": " << eventType << ": unknown exception";
      LOG(strm.str());
    }
  }

 public:
  explicit EventListener(const char *name)
      : m_creates(0),
        m_updates(0),
        m_invalidates(0),
        m_destroys(0),
        m_name(name) {}

  ~EventListener() noexcept override = default;

  void afterCreate(const EntryEvent &event) override {
    check(event, "afterCreate");
    m_creates++;
  }

  void afterUpdate(const EntryEvent &event) override {
    check(event, "afterUpdate");
    m_updates++;
  }

  void afterInvalidate(const EntryEvent &event) override {
    check(event, "afterInvalidate");
    m_invalidates++;
  }

  void afterDestroy(const EntryEvent &event) override {
    check(event, "afterDestroy");
    m_destroys++;
  }

  void reset() {
    m_creates = 0;
    m_updates = 0;
    m_invalidates = 0;
    m_destroys = 0;
  }

  // validate expected event counts
  void validate(int creates, int updates, int invalidates, int destroys) {
    LOG(std::string("VALIDATE CALLED for ") + m_name);
    auto msg = std::string("creates: expected = ") + std::to_string(creates) +
               ", actual = " + std::to_string(m_creates);
    LOG(msg);
    ASSERT(m_creates == creates, msg);
    msg = std::string("updates: expected = ") + std::to_string(updates) +
          ", actual = " + std::to_string(m_updates);
    LOG(msg);
    ASSERT(m_updates == updates, msg);
    msg = std::string("invalidates: expected = ") +
          std::to_string(invalidates) +
          ", actual = " + std::to_string(m_invalidates);
    LOG(msg);
    ASSERT(m_invalidates == invalidates, msg);
    msg = std::string("destroys: expected = ") + std::to_string(destroys) +
          ", actual = " + std::to_string(m_destroys);
    LOG(msg);
    ASSERT(m_destroys == destroys, msg);
  }
};

void setCacheListener(const char *regName,
                      std::shared_ptr<EventListener> monitor) {
  auto reg = getHelper()->getRegion(regName);
  auto attrMutator = reg->getAttributesMutator();
  attrMutator->setCacheListener(monitor);
}

// clientXXRegionYY where XX is NBS setting and YY is receiveValue setting in
// RegisterInterest API calls,
// RegionOther means no interest registered so no events should arrive except
// invalidates when NBS == false.
std::shared_ptr<EventListener> clientTrueRegionTrue = nullptr;
std::shared_ptr<EventListener> clientTrueRegionFalse = nullptr;
std::shared_ptr<EventListener> clientTrueRegionOther = nullptr;

const char *regions[] = {"RegionTrue", "RegionFalse", "RegionOther"};
const char *keysForRegex[] = {"key-regex-1", "key-regex-2", "key-regex-3"};

#include "ThinClientDurableInit.hpp"
#include "ThinClientTasks_C2S2.hpp"

void initClientForInterestNotify(std::shared_ptr<EventListener> &mon1,
                                 std::shared_ptr<EventListener> &mon2,
                                 std::shared_ptr<EventListener> &mon3,
                                 const char *clientName) {
  auto props = Properties::create();

  initClient(true, props);

  LOG("CLIENT: Setting pool with locator.");
  getHelper()->createPoolWithLocators("__TESTPOOL1_", locatorsG, true, 0,
                                      std::chrono::seconds(1));
  createRegionAndAttachPool(regions[0], USE_ACK, "__TESTPOOL1_", true);
  createRegionAndAttachPool(regions[1], USE_ACK, "__TESTPOOL1_", true);
  createRegionAndAttachPool(regions[2], USE_ACK, "__TESTPOOL1_", true);

  std::string name1 = clientName;
  name1 += "_";
  name1 += regions[0];
  std::string name2 = clientName;
  name2 += "_";
  name2 += regions[1];
  std::string name3 = clientName;
  name3 += "_";
  name3 += regions[2];

  // Recreate listeners
  mon1 = std::make_shared<EventListener>(name1.c_str());
  mon2 = std::make_shared<EventListener>(name2.c_str());
  mon3 = std::make_shared<EventListener>(name3.c_str());

  setCacheListener(regions[0], mon1);
  setCacheListener(regions[1], mon2);
  setCacheListener(regions[2], mon3);

  LOG("initClientForInterestNotify complete.");
}

void feederPuts(int count) {
  for (int region = 0; region < 3; region++) {
    for (int key = 0; key < 3; key++) {
      // if you create entry with value == 0 it does check for
      // value not exist and fails so start the entry count from 1.
      for (int entry = 1; entry <= count; entry++) {
        createIntEntry(regions[region], keys[key], entry);
        createIntEntry(regions[region], keysForRegex[key], entry);
      }
    }
  }
}

void feederInvalidates() {
  for (int region = 0; region < 3; region++) {
    for (int key = 0; key < 3; key++) {
      invalidateEntry(regions[region], keys[key]);
      invalidateEntry(regions[region], keysForRegex[key]);
    }
  }
}

void feederDestroys() {
  for (int region = 0; region < 3; region++) {
    for (int key = 0; key < 3; key++) {
      destroyEntry(regions[region], keys[key]);
      destroyEntry(regions[region], keysForRegex[key]);
    }
  }
}

void registerInterests(const char *region, bool durable, bool receiveValues) {
  auto regionPtr = getHelper()->getRegion(region);

  std::vector<std::shared_ptr<CacheableKey>> keysVector;

  keysVector.push_back(CacheableKey::create(keys[0]));
  keysVector.push_back(CacheableKey::create(keys[1]));
  keysVector.push_back(CacheableKey::create(keys[2]));

  regionPtr->registerKeys(keysVector, durable, true, receiveValues);

  regionPtr->registerRegex("key-regex.*", durable, true, receiveValues);
}

void unregisterInterests(const char *region) {
  auto regionPtr = getHelper()->getRegion(region);

  std::vector<std::shared_ptr<CacheableKey>> keysVector;

  keysVector.push_back(CacheableKey::create(keys[0]));
  keysVector.push_back(CacheableKey::create(keys[1]));
  keysVector.push_back(CacheableKey::create(keys[2]));

  regionPtr->unregisterKeys(keysVector);

  regionPtr->unregisterRegex("key-regex.*");
}

void closeClient() {
  getHelper()->disconnect();
  cleanProc();
  LOG("CLIENT CLOSED");
}

DUNIT_TASK_DEFINITION(SERVER_AND_FEEDER, StartServerWithLocator_NBS)
  {
    if (isLocalServer) {
      CacheHelper::initServer(1, "cacheserver_interest_notify.xml", locatorsG);
    }
    LOG("SERVER with NBS=false started with locator");
  }
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(SERVER_AND_FEEDER, FeederUpAndFeed)
  {
    initClientWithPool(true, "__TEST_POOL1__", locatorsG, {}, nullptr, 0, true);
    getHelper()->createPooledRegion(regions[0], USE_ACK, locatorsG,
                                    "__TEST_POOL1__", true, true);
    getHelper()->createPooledRegion(regions[1], USE_ACK, locatorsG,
                                    "__TEST_POOL1__", true, true);
    getHelper()->createPooledRegion(regions[2], USE_ACK, locatorsG,
                                    "__TEST_POOL1__", true, true);

    feederPuts(1);

    LOG("FeederUpAndFeed complete.");
  }
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(CLIENT_NBS_TRUE, ClientNbsTrue_Up)
  {
    initClientForInterestNotify(clientTrueRegionTrue, clientTrueRegionFalse,
                                clientTrueRegionOther, "clientNbsTrue");
    LOG("ClientNbsTrue_Up complete");
  }
END_TASK_DEFINITION

/*
DUNIT_TASK_DEFINITION(CLIENT_NBS_FALSE, ClientNbsFalse_Up)
{
  initClientForInterestNotify( clientFalseRegionTrue ,
  clientFalseRegionFalse, clientFalseRegionOther, "false", "clientNbsFalse" );
  LOG("ClientNbsFalse_Up complete");
}
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(CLIENT_NBS_DEFAULT, ClientNbsDefault_Up)
{
  initClientForInterestNotify( clientDefaultRegionTrue ,
  clientDefaultRegionFalse, clientDefaultRegionOther, "server",
"clientNbsDefault" );
  LOG("ClientNbsDefault_Up complete");
}
END_TASK_DEFINITION
*/

DUNIT_TASK_DEFINITION(CLIENT_NBS_TRUE, ClientNbsTrue_Register)
  {
    registerInterests(regions[0], false, true);
    registerInterests(regions[1], false, false);

    // We intentionally DO NOT  register interest  in the third region to
    // check that we don't get unexpected events based on the NBS setting.
    // registerInterests(regions[2], false, true);

    LOG("ClientNbsTrue_Register complete");
  }
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(CLIENT_NBS_TRUE, ClientNbsTrue_Unregister)
  {
    unregisterInterests(regions[0]);
    unregisterInterests(regions[1]);
    // unregisterInterests(regions[2]);
    LOG("ClientNbsTrue_Unregister complete");
  }
END_TASK_DEFINITION

/*
DUNIT_TASK_DEFINITION(CLIENT_NBS_FALSE, ClientNbsFalse_Register)
{
  registerInterests(regions[0], false, true);
  registerInterests(regions[1], false, false);

  // We intentionally DO NOT register interest in the third region to
  // check that we don't get unexpected events based on the NBS setting.
  //registerInterests(regions[2], false, true);

  LOG("ClientNbsFalse_Register complete");
}
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(CLIENT_NBS_FALSE, ClientNbsFalse_Unregister)
{
  unregisterInterests(regions[0]);
  unregisterInterests(regions[1]);
  //unregisterInterests(regions[2]);
  LOG("ClientNbsFalse_Unregister complete");
}
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(CLIENT_NBS_DEFAULT, ClientNbsDefault_Register)
{
  registerInterests(regions[0], false, true);
  registerInterests(regions[1], false, false);

  // We intentionally DO NOT  register interest  in the third region to
  // check that we don't get unexpected events based on the NBS setting.
  //registerInterests(regions[2], false, true);

  LOG("ClientNbsDefault_Register complete");
}
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(CLIENT_NBS_DEFAULT, ClientNbsDefault_Unregister)
{
  unregisterInterests(regions[0]);
  unregisterInterests(regions[1]);
  //unregisterInterests(regions[2]);
  LOG("ClientNbsDefault_Unregister complete");
}
END_TASK_DEFINITION
*/

DUNIT_TASK_DEFINITION(SERVER_AND_FEEDER, FeederDoOps)
  {
    // Do 3 puts, 1 invalidate and 1 destroy for each of the 6 keys
    feederPuts(3);
    feederInvalidates();
    feederDestroys();
    LOG("FeederDoOps complete.");
  }
END_TASK_DEFINITION

// VERIFICATION COUNTS:
// Each regon has 6 keys, for each key feeder does:
// 3 puts, 1 invalidate, 1 destroy.
// Invalidate operations from the feeder are not sent to the server so
// registered clients do not receive those invalidates only those
// that are registered interest or due to notify-by-subscription=false.
// Feeder does the above steps 3 times, once when clients have registered
// interest, then when clients unregister interest, then again when clients
// re-register interest.

DUNIT_TASK_DEFINITION(CLIENT_NBS_TRUE, ClientNbsTrue_Verify)
  {
    // validate expected creates, updates, invalidates and destroys in that
    // order
    // Verify only events received while client had registered interest
    clientTrueRegionTrue->validate(6, 30, 0, 12);
    clientTrueRegionFalse->validate(0, 0, 36, 12);
    clientTrueRegionOther->validate(0, 0, 0, 0);
    LOG("ClientNbsTrue_Verify complete");
  }
END_TASK_DEFINITION

/*
DUNIT_TASK_DEFINITION(CLIENT_NBS_FALSE, ClientNbsFalse_Verify)
{
  //validate expected creates, updates, invalidates and destroys in that order
  // Verify only events received while client had registered interest
  clientFalseRegionTrue->validate(0, 0, 54, 18);
  clientFalseRegionFalse->validate(0, 0, 54, 18);
  clientFalseRegionOther->validate(0, 0, 54, 18);
  LOG("ClientNbsFalse_Verify complete");
}
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(CLIENT_NBS_DEFAULT, ClientNbsDefault_Verify)
{
  //validate expected creates, updates, invalidates and destroys in that order
  // Verify only events received while client had registered interest
  clientDefaultRegionTrue->validate(0, 0, 54, 18);
  clientDefaultRegionFalse->validate(0, 0, 54, 18);
  clientDefaultRegionOther->validate(0, 0, 54, 18);
  LOG("ClientNbsDefault_Verify complete");
}
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(CLIENT_NBS_DEFAULT, ClientNbsDefault_Close)
{
  cleanProc();
  LOG("ClientNbsDefault_Close complete");
}
END_TASK_DEFINITION
*/

DUNIT_TASK_DEFINITION(CLIENT_NBS_TRUE, ClientNbsTrue_Close)
  {
    cleanProc();
    LOG("ClientNbsTrue_Close complete");
  }
END_TASK_DEFINITION

/*
DUNIT_TASK_DEFINITION(CLIENT_NBS_FALSE, ClientNbsFalse_Close)
{
  cleanProc();
  LOG("ClientNbsFalse_Close complete");
}
END_TASK_DEFINITION
*/

DUNIT_TASK_DEFINITION(SERVER_AND_FEEDER, CloseFeeder)
  {
    cleanProc();
    LOG("FEEDER closed");
  }
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(SERVER_AND_FEEDER, CloseServer)
  {
    CacheHelper::closeServer(1);
    LOG("SERVER closed");
  }
END_TASK_DEFINITION

DUNIT_MAIN
  {
    CALL_TASK(StartLocator);
    CALL_TASK(StartServerWithLocator_NBS);

    CALL_TASK(FeederUpAndFeed);

    CALL_TASK(ClientNbsTrue_Up);
    /*
    CALL_TASK( ClientNbsFalse_Up );
    CALL_TASK( ClientNbsDefault_Up );
    */

    CALL_TASK(ClientNbsTrue_Register);
    /*
    CALL_TASK( ClientNbsFalse_Register );
    CALL_TASK( ClientNbsDefault_Register );
    */

    // Do 3 puts, 1 invalidate and 1 destroy for
    // each of the 6 keys while client has registered interest
    CALL_TASK(FeederDoOps);

    // wait for queues to drain
    SLEEP(10000);

    CALL_TASK(ClientNbsTrue_Unregister);
    /*
    CALL_TASK( ClientNbsFalse_Unregister );
    CALL_TASK( ClientNbsDefault_Unregister );
    */

    // Do 3 puts, 1 invalidate and 1 destroy for
    // each of the 6 keys while client has UN-registered interest
    CALL_TASK(FeederDoOps);

    CALL_TASK(ClientNbsTrue_Register);
    /*
    CALL_TASK( ClientNbsFalse_Register );
    CALL_TASK( ClientNbsDefault_Register );
    */

    // Do 3 puts, 1 invalidate and 1 destroy for
    // each of the 6 keys while client has RE-registered interest
    CALL_TASK(FeederDoOps);

    // wait for queues to drain
    SLEEP(10000);

    // Verify only events received while client had registered interest
    CALL_TASK(ClientNbsTrue_Verify);
    /*
    CALL_TASK( ClientNbsFalse_Verify );
    CALL_TASK( ClientNbsDefault_Verify );
    */

    CALL_TASK(ClientNbsTrue_Close);
    /*
    CALL_TASK( ClientNbsFalse_Close );
    CALL_TASK( ClientNbsDefault_Close );
    */

    CALL_TASK(CloseFeeder);

    CALL_TASK(CloseServer);

    closeLocator();
  }
END_MAIN
