#pragma once

#include <chrono>
#include "IStorage.h"
#include "OutputObservable.h"
#include "Statistics.h"

#include <iostream>

class Storage : public IStorage, public OutputObservable, public BaseStatistics
{
public:

  explicit Storage(std::size_t block_size)
    : block_size{block_size}, is_dynamic_size{false} {}


  void Push(const std::string& new_data) override {
    if(!data) {
      timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
                  std::chrono::system_clock::now().time_since_epoch()).count();
      data = std::make_shared<std::list<std::string>>();
    }
    data->push_back(new_data);
    if(!is_dynamic_size
      && (block_size == data->size())) {
      Flush();
    }
  }

  void Flush() override {
    if(data) {
      ++statistics.blocks;
      statistics.commands += data->size();
      Output(timestamp, data);
      data.reset();
    }
  }

  void BlockStart() override {
    Flush();
    is_dynamic_size = true;
  }

  void BlockEnd() override {
    Flush();
    is_dynamic_size = false;
  }

private:
  const std::size_t block_size;
  bool is_dynamic_size;
  std::shared_ptr<std::list<std::string>> data;
  std::size_t timestamp;
};
