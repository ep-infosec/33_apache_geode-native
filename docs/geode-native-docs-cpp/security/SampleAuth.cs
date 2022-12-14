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

using System;
using Apache.Geode.Client;

namespace Apache.Geode.Examples.AuthInitialize
{
  class Program
  {
    class ExampleAuthInitialize : IAuthInitialize
    {
      public ExampleAuthInitialize()
      {
	  // TODO initialize your resources here
	  Console.Out.WriteLine("ExampleAuthInitialize::ExampleAuthInitialize called");
      }

      public void Close()
      {
	  // TODO close your resources here
	  Console.Out.WriteLine("ExampleAuthInitialize::Close called");
      }

      public Properties<string, object> GetCredentials(Properties<string, string> props, string server)
      {
	  // TODO get your username and password
	  Console.Out.WriteLine("ExampleAuthInitialize::GetCredentials called");

	  var credentials = new Properties<string, object>();
	  credentials.Insert("username", "john");
	  credentials.Insert("password", "secret");
	  return credentials;
      }
    }

    static void Main(string[] args)
    {
      var cacheFactory = new CacheFactory()
          .Set("log-level", "none")
          .SetAuthInitialize(new ExampleAuthInitialize());

      var cache = cacheFactory.Create();
      var poolFactory = cache.GetPoolFactory()
          .AddLocator("localhost", 10334);
      poolFactory.Create("pool");
      var regionFactory = cache.CreateRegionFactory(RegionShortcut.PROXY)
          .SetPoolName("pool");
      var region = regionFactory.Create<string, string>("region");

      region["a"] = "1";
      region["b"] = "2";

      var a = region["a"];
      var b = region["b"];

      Console.Out.WriteLine("a = " + a);
      Console.Out.WriteLine("b = " + b);

      cache.Close();
    }
  }
}
