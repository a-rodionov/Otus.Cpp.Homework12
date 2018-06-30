#include <algorithm>
#include <thread>
#include <chrono>
#include <boost/asio.hpp>

#define BOOST_TEST_IGNORE_NON_ZERO_CHILD_CODE
#define BOOST_TEST_MODULE test_async

#include <boost/test/unit_test.hpp>
#include <boost/test/included/unit_test.hpp>

using namespace boost::asio;
using namespace std::chrono_literals;


BOOST_AUTO_TEST_SUITE(test_suite_main)

std::string GetCurrentWorkingDir( void ) {
  char buff[FILENAME_MAX];
  ssize_t len = sizeof(buff);
  int bytes = std::min(readlink("/proc/self/exe", buff, len), len - 1);
  if(bytes >= 0)
      buff[bytes] = '\0';
  std::string current_working_dir(buff);
  return current_working_dir.substr(0, current_working_dir.find_last_of('/'));
}

void StartServer(const std::string& cmd, std::atomic_bool& isThreadStarted) {
  try {
    isThreadStarted = true;
    std::system(cmd.c_str());
  }
  catch(...) {}
}

BOOST_AUTO_TEST_CASE(single_connection_multiple_receives)
{
  std::atomic_bool isThreadStarted{false};
  std::vector<std::string> test_data {
    {"cmd1\n"}, {"cmd2\n"}, {"cmd3\n"}, {"cmd4\n"}, {"cmd5\n"}
  };
  std::vector<std::string> ethalon {
    {"bulk: cmd1, cmd2, cmd3"},
    {"bulk: cmd4, cmd5"},
  };
  std::vector<std::string> result;
  result.reserve(ethalon.size());

  auto cmd = GetCurrentWorkingDir();
  cmd += "/bulk_server 9000 3 > console.output";
  std::thread server_thread(StartServer, cmd, std::ref(isThreadStarted));
  while(!isThreadStarted) {}
  std::this_thread::sleep_for(200ms);

  boost::asio::io_service io_service;
  ip::tcp::endpoint ep( ip::address::from_string("127.0.0.1"), 9000);
  ip::tcp::socket sock(io_service);
  sock.connect(ep);

  std::for_each(std::cbegin(test_data),
                std::cend(test_data),
                [&sock](const auto& element) {
                  sock.write_some(buffer(element));
                });
  sock.close();
  std::this_thread::sleep_for(200ms);

  std::ifstream ifs{"console.output", std::ifstream::in};
  BOOST_REQUIRE_EQUAL(true, ifs.is_open());
  for(std::string line; getline(ifs, line);) {
    result.push_back(line);
  }
  ifs.close();

  BOOST_CHECK_EQUAL_COLLECTIONS(std::cbegin(ethalon),
                                std::cend(ethalon),
                                std::cbegin(result),
                                std::cend(result));

  std::system("killall bulk_server");
  server_thread.join();
  std::system("rm -f *.log");
  std::system("rm -f console.output");
}

BOOST_AUTO_TEST_CASE(multiple_connections_cmd_mix)
{
  std::atomic_bool isThreadStarted{false};
  std::vector<std::vector<std::string>> test_data {
    {{"cmd01\n"}, {"cmd02\n"}, {"cmd03\n"}},
    {{"cmd11\n"}, {"cmd12\n"}},
    {{"cmd21\n"}, {"cmd22\n"}}
  };
  std::vector<std::string> ethalon {
    {"bulk: cmd01, cmd11, cmd21"},
    {"bulk: cmd02, cmd12, cmd22"},
    {"bulk: cmd03"}
  };
  std::vector<std::string> result;
  result.reserve(ethalon.size());

  auto cmd = GetCurrentWorkingDir();
  cmd += "/bulk_server 9000 3 > console.output";
  std::thread server_thread(StartServer, cmd, std::ref(isThreadStarted));
  while(!isThreadStarted) {}
  std::this_thread::sleep_for(200ms);

  boost::asio::io_service io_service;
  ip::tcp::endpoint ep( ip::address::from_string("127.0.0.1"), 9000);
  ip::tcp::socket sock_1(io_service);
  sock_1.connect(ep);
  ip::tcp::socket sock_2(io_service);
  sock_2.connect(ep);
  ip::tcp::socket sock_3(io_service);
  sock_3.connect(ep);

  size_t maxSize = std::max_element(std::cbegin(test_data),
                                    std::cend(test_data),
                                    [](const auto& lhs, const auto& rhs)
                                      {
                                        return lhs.size() < rhs.size();
                                      })->size();
  for(decltype(maxSize) i{0}; i < maxSize; ++i) {
    if(i < test_data[0].size()) {
      sock_1.write_some(buffer(test_data[0][i]));
      std::this_thread::sleep_for(200ms);
    }
    if(i < test_data[1].size()) {
      sock_2.write_some(buffer(test_data[1][i]));
      std::this_thread::sleep_for(200ms);
    }
    if(i < test_data[2].size()) {
      sock_3.write_some(buffer(test_data[2][i]));
      std::this_thread::sleep_for(200ms);
    }
  }
  sock_1.close();
  sock_2.close();
  sock_3.close();
  std::this_thread::sleep_for(200ms);

  std::ifstream ifs{"console.output", std::ifstream::in};
  BOOST_REQUIRE_EQUAL(true, ifs.is_open());
  for(std::string line; getline(ifs, line);) {
    result.push_back(line);
  }
  ifs.close();

  BOOST_CHECK_EQUAL_COLLECTIONS(std::cbegin(ethalon),
                                std::cend(ethalon),
                                std::cbegin(result),
                                std::cend(result));
  
  std::system("killall bulk_server");
  server_thread.join();
  std::system("rm -f *.log");
  std::system("rm -f console.output");
}

BOOST_AUTO_TEST_CASE(multiple_connections_cmd_mix_with_block)
{
  std::atomic_bool isThreadStarted{false};
  std::vector<std::vector<std::string>> test_data {
    {{"cmd01\n"}, {"{\ncmd02\n"}, {"cmd03\n"}, {"}\n"}},
    {{"cmd11\n"}, {"cmd12\n"}, {"cmd13\n"}, {"cmd14\n"}},
    {{"cmd21\n"}, {"cmd22\n"}, {"cmd23\n"}}
  };
  std::vector<std::string> ethalon {
    {"bulk: cmd01, cmd11, cmd21"},
    {"bulk: cmd12, cmd22, cmd13"},
    {"bulk: cmd02, cmd03"},
    {"bulk: cmd23, cmd14"}
  };
  std::vector<std::string> result;
  result.reserve(ethalon.size());

  auto cmd = GetCurrentWorkingDir();
  cmd += "/bulk_server 9000 3 > console.output";
  std::thread server_thread(StartServer, cmd, std::ref(isThreadStarted));
  while(!isThreadStarted) {}
  std::this_thread::sleep_for(200ms);

  boost::asio::io_service io_service;
  ip::tcp::endpoint ep( ip::address::from_string("127.0.0.1"), 9000);
  ip::tcp::socket sock_1(io_service);
  sock_1.connect(ep);
  ip::tcp::socket sock_2(io_service);
  sock_2.connect(ep);
  ip::tcp::socket sock_3(io_service);
  sock_3.connect(ep);

  size_t maxSize = std::max_element(std::cbegin(test_data),
                                    std::cend(test_data),
                                    [](const auto& lhs, const auto& rhs)
                                      {
                                        return lhs.size() < rhs.size();
                                      })->size();
  for(decltype(maxSize) i{0}; i < maxSize; ++i) {
    if(i < test_data[0].size()) {
      sock_1.write_some(buffer(test_data[0][i]));
      std::this_thread::sleep_for(100ms);
    }
    if(i < test_data[1].size()) {
      sock_2.write_some(buffer(test_data[1][i]));
      std::this_thread::sleep_for(100ms);
    }
    if(i < test_data[2].size()) {
      sock_3.write_some(buffer(test_data[2][i]));
      std::this_thread::sleep_for(100ms);
    }
  }
  sock_1.close();
  sock_2.close();
  sock_3.close();
  std::this_thread::sleep_for(200ms);

  std::ifstream ifs{"console.output", std::ifstream::in};
  BOOST_REQUIRE_EQUAL(true, ifs.is_open());
  for(std::string line; getline(ifs, line);) {
    result.push_back(line);
  }
  ifs.close();

  BOOST_CHECK_EQUAL_COLLECTIONS(std::cbegin(ethalon),
                                std::cend(ethalon),
                                std::cbegin(result),
                                std::cend(result));

  std::system("killall bulk_server");
  server_thread.join();
  std::system("rm -f *.log");
  std::system("rm -f console.output");
}

BOOST_AUTO_TEST_CASE(multiple_connections_cmd_mix_with_incomplete_block)
{
  std::atomic_bool isThreadStarted{false};
  std::vector<std::vector<std::string>> test_data {
    {{"cmd01\n"}, {"{\ncmd02\n"}, {"cmd03\n"}, {"cmd04\n"}},
    {{"cmd11\n"}, {"cmd12\n"}, {"cmd13\n"}, {"cmd14\n"}},
    {{"cmd21\n"}, {"cmd22\n"}, {"cmd23\n"}}
  };
  std::vector<std::string> ethalon {
    {"bulk: cmd01, cmd11, cmd21"},
    {"bulk: cmd12, cmd22, cmd13"},
    {"bulk: cmd23, cmd14"}
  };
  std::vector<std::string> result;
  result.reserve(ethalon.size());

  auto cmd = GetCurrentWorkingDir();
  cmd += "/bulk_server 9000 3 > console.output";
  std::thread server_thread(StartServer, cmd, std::ref(isThreadStarted));
  while(!isThreadStarted) {}
  std::this_thread::sleep_for(200ms);

  boost::asio::io_service io_service;
  ip::tcp::endpoint ep( ip::address::from_string("127.0.0.1"), 9000);
  ip::tcp::socket sock_1(io_service);
  sock_1.connect(ep);
  ip::tcp::socket sock_2(io_service);
  sock_2.connect(ep);
  ip::tcp::socket sock_3(io_service);
  sock_3.connect(ep);

  size_t maxSize = std::max_element(std::cbegin(test_data),
                                    std::cend(test_data),
                                    [](const auto& lhs, const auto& rhs)
                                      {
                                        return lhs.size() < rhs.size();
                                      })->size();
  for(decltype(maxSize) i{0}; i < maxSize; ++i) {
    if(i < test_data[0].size()) {
      sock_1.write_some(buffer(test_data[0][i]));
      std::this_thread::sleep_for(100ms);
    }
    if(i < test_data[1].size()) {
      sock_2.write_some(buffer(test_data[1][i]));
      std::this_thread::sleep_for(100ms);
    }
    if(i < test_data[2].size()) {
      sock_3.write_some(buffer(test_data[2][i]));
      std::this_thread::sleep_for(100ms);
    }
  }
  sock_1.close();
  sock_2.close();
  sock_3.close();
  std::this_thread::sleep_for(200ms);

  std::ifstream ifs{"console.output", std::ifstream::in};
  BOOST_REQUIRE_EQUAL(true, ifs.is_open());
  for(std::string line; getline(ifs, line);) {
    result.push_back(line);
  }
  ifs.close();

  BOOST_CHECK_EQUAL_COLLECTIONS(std::cbegin(ethalon),
                                std::cend(ethalon),
                                std::cbegin(result),
                                std::cend(result));
  
  std::system("killall bulk_server");
  server_thread.join();
  std::system("rm -f *.log");
  std::system("rm -f console.output");
}

BOOST_AUTO_TEST_CASE(remove_test_files)
{
  std::system("rm -f *.log");
}

BOOST_AUTO_TEST_SUITE_END()
