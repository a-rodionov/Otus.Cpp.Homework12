#pragma once

#include "IStorage.h"
#include "Observable.h"

auto flush_notification = [] (const std::shared_ptr<IStorage>& subscriber) { subscriber->Flush(); };
auto block_start_notification = [] (const std::shared_ptr<IStorage>& subscriber) { subscriber->BlockStart(); };
auto block_end_notification = [] (const std::shared_ptr<IStorage>& subscriber) { subscriber->BlockEnd(); };

class StorageObservable : public Observable<IStorage>
{

protected:

  void Push(const std::string& data) {
    Notify( [&data] (const std::shared_ptr<IStorage>& subscriber) { subscriber->Push(data); } );
  }

  void Flush() {
    Notify(flush_notification);
  }

  void BlockStart() {
    Notify(block_start_notification);
  }

  void BlockEnd() {
    Notify(block_end_notification);  
  }

private:

  template<typename Callable>
  void Notify(Callable&& callable) {
     for (const auto& subscriber : subscribers) {
      auto subscriber_locked = subscriber.lock();
      if(subscriber_locked) {
        callable(subscriber_locked);
      }
    }  
  }

};
