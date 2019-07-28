// Copyright (C) 2019 Vincent Chambrin
// This file is part of the yaml project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef YAML_H
#define YAML_H

#include "yaml/yaml-defs.h"

#include "json-toolkit/json.h"

namespace yaml
{

YAML_API json::Json parse(const std::string& str);

} // namespace yaml

#endif // !YAML_H
