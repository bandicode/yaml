// Copyright (C) 2019 Vincent Chambrin
// This file is part of the yaml project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "yaml/yaml.h"

namespace yaml
{

struct Stream
{
  int m_pos = 0;
  int m_length = 0;
  const std::string* m_text = nullptr;

  int pos() const { return m_pos; }
  int length() const { return m_length; }

  const std::string& text() const { return *m_text; }

  void reset(const std::string& str)
  {
    m_pos = 0;
    m_length = StringBackend::strlength(str);
    m_text = &str;
  }

  bool atEnd() const { return m_pos == m_length; }

  std::string  readLine()
  {
    int n = StringBackend::index_of(StringBackend::new_line(), text(), pos());
    std::string  result = StringBackend::mid(text(), pos(), n);
    m_pos = (n == -1 ? length() : m_pos + n + 1);
    return result;
  }
};

class Parser
{
private:
  Stream m_stream;
  std::string  m_currentline;
  int m_currentindent = 0;
  bool m_done = false;

public:
  Parser() = default;

  json::Json parse(const std::string& str);

protected:
  Stream& stream() { return m_stream; }
  const Stream& stream() const { return m_stream; }
  const std::string& currentLine() const { return m_currentline; }
  int currentIndent() const { return m_currentindent; }

private:
  bool hasNext() const;
  void getNext();
  void nextOrDone();

  json::Json parseValue(const int indent);

  static std::vector<std::string > extractFields(const std::string& str);
  static json::Object parseObject(std::string str);
  static json::Array parseArray(std::string str);
  static json::Json parseString(std::string str);

  static int computeIndent(const std::string& str);
  static int readPropertyName(const std::string& str, int indent);
};

json::Json Parser::parse(const std::string& str)
{
  m_done = false;
  m_stream.reset(str);
  if (stream().atEnd())
    return nullptr;

  getNext();
  return parseValue(0);
}

bool Parser::hasNext() const
{
  return !stream().atEnd();
}

void Parser::getNext()
{
  m_currentline = stream().readLine();
  while (m_currentline.size() > 0 && StringBackend::is_space(m_currentline.back()))
    StringBackend::chop(m_currentline, 1);
  m_currentindent = computeIndent(m_currentline);
}

void Parser::nextOrDone()
{
  if (hasNext())
  {
    getNext();
  }
  else
  {
    m_done = true;
    m_currentindent = -1;
  }
}

json::Json Parser::parseValue(const int indent)
{
  if (currentLine().at(currentIndent() * 2) == '-')
  {
    // Parse list
    json::Array result{};

    do
    {
      assert(currentLine().at(currentIndent() * 2) == '-');

      /// TODO: problem
      //currentline[currentindent * 2] = ' ';
      m_currentindent += 1;
      json::Json elem = parseValue(indent + 1);
      result.push(elem);
    } while (m_currentindent == indent);

    return result;
  }
  else if (currentLine().at(currentIndent() * 2) == '{')
  {
    // Inline object
    json::Json ret = parseObject(StringBackend::trimmed(StringBackend::mid(currentLine(), currentIndent() * 2)));
    nextOrDone();
    return ret;
  }
  else if (currentLine().at(currentIndent() * 2) == '[')
  {
    // Inline array
    json::Json ret = parseArray(StringBackend::trimmed(StringBackend::mid(currentLine(), currentIndent() * 2)));
    nextOrDone();
    return ret;
  }

  int colon = readPropertyName(currentLine(), currentIndent());
  if (colon != -1)
  {
    // Parse object
    json::Object result{};

    do
    {
      int colon = readPropertyName(currentLine(), currentIndent());
      std::string  property_name = StringBackend::mid(currentLine(), currentIndent() * 2, colon - currentIndent() * 2);
      json::Json property_value{ nullptr };

      if (currentLine().back() == ':')
      {
        getNext();
        property_value = parseValue(indent + 1);
      }
      else if (currentLine().back() == '}' && currentLine().at(colon + 2) == '{')
      {
        // name: {...}
        const int opening_brace = StringBackend::index_of('{', currentLine(), colon);
        std::string  subobject = StringBackend::mid(currentLine(), opening_brace);
        property_value = parseObject(subobject);

        nextOrDone();
      }
      else if (currentLine().back() == ']' && currentLine().at(colon + 2) == '[')
      {
        // name: [...]
        const int opening_bracket = StringBackend::index_of('[', currentLine(), colon);
        std::string  subarray = StringBackend::mid(currentLine(), opening_bracket);
        property_value = parseArray(subarray);

        nextOrDone();
      }
      else
      {
        // name: value
        property_value = parseString(StringBackend::mid(currentLine(), colon + 1));
        nextOrDone();
      }

      result[property_name] = property_value;

    } while (currentIndent() == indent);

    return result;
  }
  else
  {
    // Parse string
    auto ret = parseString(StringBackend::mid(currentLine(), currentIndent()));
    nextOrDone();
    return ret;
  }
}

std::vector<std::string > Parser::extractFields(const std::string& str)
{
  std::vector<std::string > result;

  int begin = 0;
  int it = 0;

  bool inside_quotes = false;

  while (it != str.length())
  {
    auto c = str.at(it);
    if (c == ',' && !inside_quotes)
    {
      result.push_back(StringBackend::mid(str, begin, it - begin));
      begin = it + 1;
    }
    else if (c == '"')
    {
      inside_quotes = !inside_quotes;
    }

    ++it;
  }

  assert(!inside_quotes);
  result.push_back(StringBackend::mid(str, begin, it - begin));
  return result;
}

json::Object Parser::parseObject(std::string  str)
{
  // Remove '}'
  StringBackend::chop(str, 1);
  // Remove '{'
  str = StringBackend::mid(str, 1);

  json::Object result{};

  std::vector<std::string > fields = extractFields(str);

  for (std::string f : fields)
  {
    f = StringBackend::trimmed(f);
    const int colon = StringBackend::index_of(':', f);
    std::string  property_name = StringBackend::mid(f, 0, colon);
    json::Json property_value = parseString(StringBackend::mid(f, colon + 2));

    result[property_name] = property_value;
  }

  return result;
}

json::Array Parser::parseArray(std::string  str)
{
  // Remove ']'
  StringBackend::chop(str, 1);
  // Remove '['
  str = StringBackend::mid(str, 1);

  json::Array result{};

  std::vector<std::string > fields = extractFields(str);

  for (std::string f : fields)
  {
    f = StringBackend::trimmed(f);
    result.push(parseString(f));
  }

  return result;
}

json::Json Parser::parseString(std::string  str)
{
  str = StringBackend::trimmed(str);

  if (str.back() == '"')
  {
    StringBackend::chop(str, 1);
    str = StringBackend::mid(str, 1);
  }

  return json::Json{ str };
}

int Parser::computeIndent(const std::string& str)
{
  size_t i = 0;
  while (i < str.length() && StringBackend::is_space(str.at(i)))
    ++i;
  return static_cast<int>(i) / 2;
}

int Parser::readPropertyName(const std::string& str, int indent)
{
  size_t i = 2 * indent;
  while (i < str.length() && (StringBackend::is_letter_or_number(str.at(i))))
    ++i;

  if (i == str.length() || str.at(i) != ':')
    return -1;
  return static_cast<int>(i);
}

json::Json parse(const std::string& str)
{
  Parser p;
  return p.parse(str);
}

} // namespace yaml

