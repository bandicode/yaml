// Copyright (C) 2019 Vincent Chambrin
// This file is part of the yaml project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "yaml/yaml.h"

#include <json-toolkit/parsing.h>
#include <json-toolkit/stringify.h>

#include <gtest/gtest.h>

TEST(Yaml, person) {

  std::string str = 
    "name: Bob\n"
    "age: 20\n"
    "friends: ['Alice', 'Eve']\n";

  json::Json val = yaml::parse(str);

  json::Array friends;
  friends.push("Alice");
  friends.push("Eve");

  ASSERT_TRUE(val.isObject());
  ASSERT_EQ(val["name"], "Bob");
  ASSERT_EQ(val["age"], 20);
  ASSERT_EQ(val["friends"], friends);
}

TEST(Yaml, string_list) {

  std::string str =
    "- Bob\n"
    "- Alice\n"
    "- Eve\n";

  json::Json val = yaml::parse(str);

  json::Array friends;
  friends.push("Bob");
  friends.push("Alice");
  friends.push("Eve");

  ASSERT_TRUE(val.isArray());
  ASSERT_EQ(val, friends);
}

TEST(Yaml, nested_json) {

  std::string str =
    "object: {'name': 'Bob', 'age': 20}\n";

  json::Json val = yaml::parse(str);

  json::Object obj;
  obj["name"] = "Bob";
  obj["age"] = 20;

  ASSERT_TRUE(val.isObject());
  ASSERT_EQ(val["object"], obj);
}

TEST(Yaml, nested_object) {

  std::string str =
    "first:\n"
    "  - 1\n"
    "  - 2\n"
    "  - 3\n"
    "second:\n"
    "  name: Bob\n"
    "  age: 20\n";

  json::Json val = yaml::parse(str);

  json::Array first = json::parse("[1, 2, 3]").toArray();

  json::Object second;
  second["name"] = "Bob";
  second["age"] = 20;

  json::Object obj;
  obj["first"] = first;
  obj["second"] = second;

  //std::cout << json::stringify(val);

  ASSERT_TRUE(val.isObject());
  ASSERT_EQ(val, obj);
}
