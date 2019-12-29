// Copyright (C) 2019 Vincent Chambrin
// This file is part of the yaml project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "yaml/yaml.h"

#include <gtest/gtest.h>

TEST(Yaml, hello) {

  std::string str = 
    "name: Bob\n"
    "age: 20";

  json::Json val = yaml::parse(str);

  ASSERT_TRUE(val.isObject());
  ASSERT_EQ(val["name"], "Bob");
  ASSERT_EQ(val["age"], "20");
}
