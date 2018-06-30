#pragma once

#include "IOutput.h"
#include "Observable.h"

class OutputObservable : public Observable<IOutput>
{

protected:

  void Output(const std::size_t timestamp, std::shared_ptr<const std::list<std::string>> data) {
    for (const auto& subscriber : subscribers) {
      auto subscriber_locked = subscriber.lock();
      if(subscriber_locked) {
        try {
          subscriber_locked->Output(timestamp, data);
        }
        catch(...) {}
      }
    }
  }

};
