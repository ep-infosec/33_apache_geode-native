# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.
# The ASF licenses this file to You under the Apache License, Version 2.0
# (the "License"); you may not use this file except in compliance with
# the License.  You may obtain a copy of the License at
# 
#      http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
cmake_minimum_required (VERSION 3.4)
project (javaobject)

find_package(Geode REQUIRED)
find_package(Java REQUIRED)

include(UseJava)
include(CheckJavaClassExists)

set(CMAKE_JAVA_COMPILE_FLAGS -encoding utf8)

check_java_class_exists(org.apache.geode.security.AuthenticationExpiredException ${Geode_CLASSPATH} hasAuthenticationExpiredException)

file(GLOB_RECURSE SOURCES "*.java")

# Check for versions of GEODE that don't support AuthenticationExpiredException
if (NOT ${hasAuthenticationExpiredException})
  list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/SimulatedExpirationSecurityManager.java)
endif()

add_jar(javaobject ${SOURCES}
  INCLUDE_JARS ${Geode_CLASSPATH}
)

set(EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/javaobject.jar)
