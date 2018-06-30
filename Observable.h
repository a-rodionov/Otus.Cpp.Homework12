#pragma once

#include <memory>
#include <list>

template<typename T>
class Observable
{
public:

  void Subscribe(const std::shared_ptr<T>& subscriber) {
    auto subscr = Find(subscriber);
    if(std::cend(subscribers) == subscr) {
      subscribers.push_back(subscriber);
    }
  }

  void Unsubscribe(const std::shared_ptr<T>& subscriber) {
    auto subscr = Find(subscriber);
    if(std::cend(subscribers) != subscr) {
      subscribers.erase(subscr);
    }
  }

protected:

  std::list<std::weak_ptr<T>> subscribers;

private:

  auto Find(const std::shared_ptr<T>& subscriber) {
    for(auto subscr = std::cbegin(subscribers); subscr != std::cend(subscribers); ++subscr) {
      auto subscriber_locked = subscr->lock();
      if(subscriber_locked == subscriber) {
        return subscr;
      }
    }
    return std::cend(subscribers);
  }
};