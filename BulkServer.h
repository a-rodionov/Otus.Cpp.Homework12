#pragma once

#include "SeparateStorage.h"
#include "ConsoleOutput.h"
#include "FileOutput.h"
#include "CommandProcessor.h"
#include <set>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

class bulk_session;
using session_set = std::shared_ptr<std::set<std::shared_ptr<bulk_session>>>;

class bulk_session :
    public std::enable_shared_from_this<bulk_session>
{
public:
  bulk_session(tcp::socket socket,
               session_set sessions,
               std::shared_ptr<Storage>& cmdStorage,
               std::shared_ptr<ConsoleOutput>& consoleOutput,
               std::shared_ptr<FileOutput>& fileOutput)
    : socket(std::move(socket)),
      sessions(sessions),
      commandProcessor(std::make_shared<CommandProcessor>()),
      cmdStorage{cmdStorage},
      blockStorage(std::make_shared<Storage>(0)),
      separateStorage(std::make_shared<SeparateStorage>(cmdStorage, blockStorage))
  {
    blockStorage->Subscribe(consoleOutput);
    blockStorage->Subscribe(fileOutput);
    commandProcessor->Subscribe(separateStorage);
  }

  void start()
  {
    sessions->insert(shared_from_this());
    do_read();
  }

private:
  void do_read()
  {
    boost::asio::async_read_until(socket, buffer, '\n', 
        [this](boost::system::error_code ec, std::size_t length)
        {
          if (!ec)
          {
            std::istream is(&buffer);
            std::string line;
            std::getline(is, line);
            line += "\n";
            commandProcessor->Process(line.c_str(), line.size());
            do_read();
          }
          else
          {
            sessions->erase(shared_from_this());
            if(0 == sessions->size())
              cmdStorage->Flush();
          }
        });
  } 

  boost::asio::streambuf buffer;
  tcp::socket socket;

  session_set sessions;

  std::shared_ptr<CommandProcessor> commandProcessor;
  std::shared_ptr<Storage> cmdStorage;
  std::shared_ptr<Storage> blockStorage;
  std::shared_ptr<SeparateStorage> separateStorage;
};

class bulk_server
{
public:
  bulk_server(boost::asio::io_service& io_service,
      const tcp::endpoint& endpoint,
      unsigned long long block_size)
    : acceptor(io_service, endpoint),
      socket(io_service),
      sessions(std::make_shared<std::set<std::shared_ptr<bulk_session>>>()),
      cmdStorage(std::make_shared<Storage>(block_size)),
      consoleOutput(std::make_shared<ConsoleOutput>(std::cout)),
      fileOutput(std::make_shared<FileOutput>(2))
  {
    cmdStorage->Subscribe(consoleOutput);
    cmdStorage->Subscribe(fileOutput);
    do_accept();
  }

private:
  void do_accept()
  {
    acceptor.async_accept(socket,
      [this](boost::system::error_code ec)
      {
        if (!ec) {
          std::make_shared<bulk_session>(std::move(socket),
                                         sessions,
                                         cmdStorage,
                                         consoleOutput,
                                         fileOutput)->start();
        }
        do_accept();
      });
  }

  tcp::acceptor acceptor;
  tcp::socket socket;
  session_set sessions;

  std::shared_ptr<Storage> cmdStorage;
  std::shared_ptr<ConsoleOutput> consoleOutput;
  std::shared_ptr<FileOutput> fileOutput;
};
