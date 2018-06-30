#pragma once

#include <iostream>
#include "StorageObservable.h"

class CommandProcessor : public StorageObservable
{

public:

  void Process(const char* data, std::size_t size, bool isFinal = false) {
    processingBuffer.append(data, data + size);

    for(auto endOfLine = processingBuffer.find('\n');
        endOfLine != std::string::npos;
        processingBuffer = processingBuffer.substr(++endOfLine),
        endOfLine = processingBuffer.find('\n')) {
      std::string command = processingBuffer.substr(0, endOfLine);
      ++processedLines;
      if("{" == command) {
        if(0 == open_brace_count++) {
          BlockStart();
        }
        continue;
      }
      if("}" == command) {
        if(0 == open_brace_count){
          BlockEnd();
        }
        else if(0 == --open_brace_count) {
          BlockEnd();
        }
        continue;
      }
      Push(command);
    }

    if(isFinal && (0 == open_brace_count)) {
      Flush();
    }
  }

  auto GetProcessedLines() const {
    return processedLines;
  }

private:

  std::size_t open_brace_count{0};
  std::size_t processedLines{0};
  std::string processingBuffer;

};
