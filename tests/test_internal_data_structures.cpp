#include "infix_iterator.h"
#include "ConsoleOutput.h"
#include "Observable.h"

#define BOOST_TEST_MODULE test_internal_data_structures

#include <boost/test/unit_test.hpp>
#include <boost/test/included/unit_test.hpp>


BOOST_AUTO_TEST_SUITE(test_suite_main)

BOOST_AUTO_TEST_CASE(infix_iterator)
{
  std::list<int> data{0,1,2,3};
  std::string result{"0, 1, 2, 3"};
  std::ostringstream oss;

  std::copy(std::cbegin(data),
            std::cend(data),
            infix_ostream_iterator<decltype(data)::value_type>{oss, ", "});
  BOOST_CHECK_EQUAL(oss.str(), result);
}

BOOST_AUTO_TEST_CASE(observable)
{
  class TestObservable : public Observable<IOutput>
  {
  public:
    auto GetSubscribers() const {
      return subscribers;
    }
    auto GetSubscribersCount() const {
      return subscribers.size();
    }
  };

  TestObservable testObservable;
  auto consoleOutput = std::make_shared<ConsoleOutput>(std::cout);
  bool isSubscriberFound{false};

  BOOST_CHECK_EQUAL(0, testObservable.GetSubscribersCount());
  
  testObservable.Subscribe(consoleOutput);
  BOOST_CHECK_EQUAL(1, testObservable.GetSubscribersCount());

  auto subscribers = testObservable.GetSubscribers();
  for(auto subscr = std::cbegin(subscribers); subscr != std::cend(subscribers); ++subscr) {
    auto subscriber_locked = subscr->lock();
    if(subscriber_locked.get() == consoleOutput.get()) {
      isSubscriberFound = true;
      break;
    }
  }
  BOOST_CHECK_EQUAL(true, isSubscriberFound);

  testObservable.Unsubscribe(consoleOutput);
  BOOST_CHECK_EQUAL(0, testObservable.GetSubscribersCount());
}

BOOST_AUTO_TEST_SUITE_END()
