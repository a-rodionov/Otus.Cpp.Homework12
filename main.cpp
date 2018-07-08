#include <algorithm>
#include <limits>
#include "BulkServer.h"
#include "Logger.h"
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

int main(int argc, char const* argv[])
{
  try
  {
    unsigned long long port_num, block_size, max_cmds_in_files;
    try
    {
      if((3 > argc)
        || (4 < argc))
       {
        throw std::invalid_argument("");
      }

      std::string digit_str{argv[1]};
      if(!std::all_of(std::cbegin(digit_str),
                      std::cend(digit_str),
                      [](unsigned char symbol) { return std::isdigit(symbol); } )) {
        throw std::invalid_argument("");
      }
      port_num = std::stoull(digit_str);
      if(port_num > std::numeric_limits<unsigned short>::max()) {
        throw std::invalid_argument("");
      }

      digit_str = argv[2];
      if(!std::all_of(std::cbegin(digit_str),
                      std::cend(digit_str),
                      [](unsigned char symbol) { return std::isdigit(symbol); } )) {
        throw std::invalid_argument("");
      }

      block_size = std::stoull(digit_str);
      if(0 == block_size) {
        throw std::invalid_argument("");
      }

      if(4 == argc) {
        digit_str = argv[3];
        if(!std::all_of(std::cbegin(digit_str),
                        std::cend(digit_str),
                        [](unsigned char symbol) { return std::isdigit(symbol); } )) {
          throw std::invalid_argument("");
        }
        max_cmds_in_files = std::stoull(digit_str);
      }
      else
        max_cmds_in_files = 0;
    }
    catch(...)
    {
      std::string error_msg = "The programm must be started with 2 or 3 parameters. First parameter is port number, "
                              "which value must be in range 0 - "
                              + std::to_string(std::numeric_limits<unsigned short>::max())
                              + ". Second parameter is block size, which value must be in range 1 - "
                              + std::to_string(std::numeric_limits<decltype(block_size)>::max())
                              + ". Third parameter is optional. It's the number of processed commands after which "
                              "an exception will be thrown in the thread that writes commands to files. If value is "
                              "0, it'll be ignored.";
      throw std::invalid_argument(error_msg);
    }

    Logger::Instance();

    bulk_server server{port_num, block_size, max_cmds_in_files};
    server.start();
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }
  return 0;
}
