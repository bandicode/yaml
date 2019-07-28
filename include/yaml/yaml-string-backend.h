// Copyright (C) 2019 Vincent Chambrin
// This file is part of the yaml project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef YAML_STRINGBACKEND_H
#define YAML_STRINGBACKEND_H

#include <iterator>
#include <string>

namespace yaml
{

class StringBackend
{
public:
  typedef std::string string_type;
  typedef std::string string_view_type;
  typedef char char_type;

  static int to_integer(const string_type& str) { return std::stoi(str); }
  static string_type from_integer(int n) { return std::to_string(n); }
  static string_type from_number(double x) { return std::to_string(x); }

  static int strlength(const string_type& str) { return static_cast<int>(str.size()); }

  static int compare(const string_type& lhs, const string_type& rhs)
  {
    return lhs.compare(rhs);
  }

  static int index_of(char_type c, const string_type& str, int from = 0) 
  { 
    size_t index = str.find(c, from);
    return index == std::string::npos ? -1 : static_cast<int>(index);
  }

  static int index_of(const string_type& pattern, const string_type& str, int from = 0)
  {
    size_t index = str.find(pattern, from);
    return index == std::string::npos ? -1 : static_cast<int>(index);
  }

  static string_view_type mid_ref(const string_type& str, int offset, int length)
  {
    return std::string(str.begin() + offset, str.begin() + offset + length);
  }

  static string_type mid(const string_type& str, int offset, int length = -1)
  {
    length = length != -1 ? length : str.size() - offset;
    return std::string(str.begin() + offset, str.begin() + offset + length);
  }

  static void chop(string_type& str, int num)
  {
    while (num > 0)
    {
      str.pop_back();
      --num;
    }
  }

  static string_type trimmed(const string_type& str)
  {
    size_t i = 0;
    while (i < str.size() && is_space(str.at(i))) ++i;

    size_t j = str.size();
    while (j-- > 0 && is_space(str.at(j)));

    if (j == std::numeric_limits<size_t>::max())
      return {};

    return std::string(str.begin() + i, str.begin() + j + 1);
  }

  static string_type normalize(const string_type& str)
  {
    std::string result = str;
    size_t pos = result.find("\r\n");
    while (pos != std::string::npos)
    {
      result.replace(pos, 2, "\n");
      pos = result.find("\r\n", pos);
    }
    return result;
  }


  static char_type new_line() { return '\n'; }

  static bool is_space(char_type c) { return c == ' ' || c == '\t'; }
  static bool is_newline(char_type c) { return c == new_line(); }
  static bool is_digit(char_type c) { return c >= '0' && c <= '9'; }
  static bool is_letter(char_type c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
  static bool is_letter_or_number(char_type c) { return is_digit(c) || is_letter(c); }
};

} // namespace yaml

#endif // !YAML_STRINGBACKEND_H
