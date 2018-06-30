#pragma once

#include <string>

class IStorage
{
public:
  virtual void Push(const std::string& data) = 0;
  virtual void Flush() = 0;
  virtual void BlockStart() = 0;
  virtual void BlockEnd() = 0;
};
