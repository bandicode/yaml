// Copyright (C) 2019 Vincent Chambrin
// This file is part of the yaml project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "yaml/yaml.h"

namespace yaml
{

inline static bool is_space(char c)
{
  return c == ' ' || c == '\t';
}

inline static bool is_digit(char c) { return c >= '0' && c <= '9'; }
inline static bool is_letter(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
inline static bool is_letter_or_number(char c) { return is_digit(c) || is_letter(c); }

static std::string trimmed(const std::string& str)
{
  size_t i = 0;
  while (i < str.size() && is_space(str.at(i))) ++i;

  size_t j = str.size();
  while (j-- > 0 && is_space(str.at(j)));

  if (j == std::numeric_limits<size_t>::max())
    return {};

  return std::string(str.begin() + i, str.begin() + j + 1);
}

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
    m_length = static_cast<int>(str.size());
    m_text = &str;
  }

  bool atEnd() const { return m_pos == m_length; }

  std::string  readLine()
  {
    size_t n = text().find('\n', pos());

    if (n == std::string::npos)
      n = text().size();

    std::string result{ text().begin() + pos(), text().begin() + n };
    m_pos = n == text().size() ? n : (n+1);

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

  static std::vector<std::string> extractFields(const std::string& str);
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
  while (m_currentline.size() > 0 && is_space(m_currentline.back()))
    m_currentline.pop_back();
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
    auto substr = std::string(currentLine().begin() + currentIndent() * 2, currentLine().end());
    json::Json ret = parseObject(trimmed(substr));
    nextOrDone();
    return ret;
  }
  else if (currentLine().at(currentIndent() * 2) == '[')
  {
    // Inline array
    auto substr = std::string(currentLine().begin() + currentIndent() * 2, currentLine().end());
    json::Json ret = parseArray(trimmed(substr));
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
      std::string  property_name = std::string(currentLine().begin() + currentIndent() * 2, currentLine().begin() + colon);
      json::Json property_value{ nullptr };

      if (currentLine().back() == ':')
      {
        getNext();
        property_value = parseValue(indent + 1);
      }
      else if (currentLine().back() == '}' && currentLine().at(colon + 2) == '{')
      {
        // name: {...}
        const size_t opening_brace = currentLine().find('{', colon);
        std::string subobject = std::string(currentLine().begin() + opening_brace, currentLine().end());
        property_value = parseObject(subobject);

        nextOrDone();
      }
      else if (currentLine().back() == ']' && currentLine().at(colon + 2) == '[')
      {
        // name: [...]
        const size_t opening_bracket = currentLine().find('[', colon);
        std::string  subarray = std::string(currentLine().begin() + opening_bracket, currentLine().end());
        property_value = parseArray(subarray);

        nextOrDone();
      }
      else
      {
        // name: value
        property_value = parseString(std::string(currentLine().begin() + colon + 1, currentLine().end()));
        nextOrDone();
      }

      result[property_name] = property_value;

    } while (currentIndent() == indent);

    return result;
  }
  else
  {
    // Parse string
    auto ret = parseString(std::string(currentLine().begin() + currentIndent(), currentLine().end()));
    nextOrDone();
    return ret;
  }
}

std::vector<std::string> Parser::extractFields(const std::string& str)
{
  std::vector<std::string> result;

  size_t begin = 0;
  size_t it = 0;

  bool inside_quotes = false;

  while (it != str.length())
  {
    auto c = str.at(it);
    if (c == ',' && !inside_quotes)
    {
      result.push_back(std::string(str.begin() + begin, str.begin() + it));
      begin = it + 1;
    }
    else if (c == '"')
    {
      inside_quotes = !inside_quotes;
    }

    ++it;
  }

  assert(!inside_quotes);
  result.push_back(std::string(str.begin() + begin, str.begin() + it));
  return result;
}

json::Object Parser::parseObject(std::string str)
{
  // Remove '{' and '}'
  str = std::string(str.begin() + 1, str.end() - 1);

  json::Object result{};

  std::vector<std::string> fields = extractFields(str);

  for (std::string f : fields)
  {
    f = trimmed(f);
    const size_t colon = f.find(':');
    std::string property_name = std::string(f.begin(), f.begin() + colon);
    json::Json property_value = parseString(std::string(f.begin() + colon + 2, f.end()));

    result[property_name] = property_value;
  }

  return result;
}

json::Array Parser::parseArray(std::string str)
{
  // Remove '[' and ']'
  str = std::string(str.begin() + 1, str.end() - 1);

  json::Array result{};

  std::vector<std::string> fields = extractFields(str);

  for (std::string f : fields)
  {
    f = trimmed(f);
    result.push(parseString(f));
  }

  return result;
}

json::Json Parser::parseString(std::string str)
{
  str = trimmed(str);

  if (str.back() == '"')
  {
    str = std::string(str.begin() + 1, str.end() - 1);
  }

  return json::Json{ str };
}

int Parser::computeIndent(const std::string& str)
{
  size_t i = 0;
  while (i < str.length() && is_space(str.at(i)))
    ++i;
  return static_cast<int>(i) / 2;
}

int Parser::readPropertyName(const std::string& str, int indent)
{
  size_t i = 2 * indent;
  while (i < str.length() && is_letter_or_number(str.at(i)))
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

