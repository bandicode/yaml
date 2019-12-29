// Copyright (C) 2019 Vincent Chambrin
// This file is part of the yaml project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef YAML_GLOBAL_DEFS_H
#define YAML_GLOBAL_DEFS_H

#if (defined(WIN32) || defined(_WIN32)) && !defined(YAML_BUILD_STATIC_LIBRARY)
#if defined(YAML_BUILD_SHARED_LIBRARY)
#  define YAML_API __declspec(dllexport)
#else
#  define YAML_API __declspec(dllimport)
#endif
#else
#define YAML_API
#endif

#endif // YAML_GLOBAL_DEFS_H
