#pragma once

#include "IStorage.h"
#include "Storage.h"

class SeparateStorage : public IStorage
{
public:

  SeparateStorage(std::shared_ptr<Storage>& cmdStorage, std::shared_ptr<Storage>& blockStorage)
    : isBlockProcessing{false}, cmdStorage{cmdStorage}, blockStorage{blockStorage} {}


  void Push(const std::string& new_data) override {
    if(isBlockProcessing)
      blockStorage->Push(new_data);
    else
      cmdStorage->Push(new_data);
  }

  void Flush() override {
  }

  void BlockStart() override {
    isBlockProcessing = true;
    blockStorage->BlockStart();
  }

  void BlockEnd() override {
    isBlockProcessing = false;
    blockStorage->BlockEnd();
  }

private:
  bool isBlockProcessing;
  std::shared_ptr<Storage> cmdStorage;
  std::shared_ptr<Storage> blockStorage;
};
