#pragma once

#include "SeparateStorage.h"
#include "ConsoleOutput.h"
#include "FileOutput.h"
#include "CommandProcessor.h"
#include <set>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

class client_session;
using session_set = std::shared_ptr<std::set<std::shared_ptr<client_session>>>;

class client_session : public std::enable_shared_from_this<client_session>, boost::noncopyable
{

  client_session(boost::asio::io_service& io_service,
                 session_set sessions,
                 std::shared_ptr<Storage>& cmdStorage,
                 std::shared_ptr<ConsoleOutput>& consoleOutput,
                 std::shared_ptr<FileOutput>& fileOutput)
    : socket(io_service),
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

public:

  ~client_session() {
    stop();
  }

  static auto make(boost::asio::io_service& io_service,
                   session_set sessions,
                   std::shared_ptr<Storage>& cmdStorage,
                   std::shared_ptr<ConsoleOutput>& consoleOutput,
                   std::shared_ptr<FileOutput>& fileOutput)
  {
    return std::shared_ptr<client_session>(new client_session(io_service,
                                                              sessions,
                                                              cmdStorage,
                                                              consoleOutput,
                                                              fileOutput));
  }

  tcp::socket& sock() {
    return socket;
  }

  void start() {
    if (isStarted)
      return;
    isStarted = true;
    do_read();
  }

  void stop() {
    if (!isStarted)
      return;
    isStarted = false;
    socket.close();
    auto itr = std::find(std::cbegin(*sessions), std::cend(*sessions), shared_from_this());
    sessions->erase(itr);
    if(0 == sessions->size())
      cmdStorage->Flush();
  }

private:

  void do_read() {
    auto self = shared_from_this();
    boost::asio::async_read_until(socket, buffer, '\n', 
        [self](boost::system::error_code ec, std::size_t length)
        {
          if (ec) {
            BOOST_LOG_TRIVIAL(error) << ec.message();
            Logger::Instance().Flush();
            self->stop();
            return;
          }

          std::istream is(&self->buffer);
          std::string line;
          std::getline(is, line);
          line += "\n";
          self->commandProcessor->Process(line.c_str(), line.size());
          self->do_read();
        });
  } 

  bool isStarted{false};
  boost::asio::streambuf buffer;
  tcp::socket socket;

  session_set sessions;

  std::shared_ptr<CommandProcessor> commandProcessor;
  std::shared_ptr<Storage> cmdStorage;
  std::shared_ptr<Storage> blockStorage;
  std::shared_ptr<SeparateStorage> separateStorage;
};

class bulk_server : public std::enable_shared_from_this<bulk_server>, boost::noncopyable
{

  bulk_server(unsigned short port_num,
              unsigned long long block_size,
              unsigned long long max_cmds_in_files)
    : io_service(),
      acceptor(io_service, tcp::endpoint{tcp::v4(), port_num}),
      sessions(std::make_shared<std::set<std::shared_ptr<client_session>>>()),
      cmdStorage(std::make_shared<Storage>(block_size)),
      consoleOutput(std::make_shared<ConsoleOutput>(std::cout)),
      fileOutput(std::make_shared<FileOutput>(2, false, max_cmds_in_files))
  {
    cmdStorage->Subscribe(consoleOutput);
    cmdStorage->Subscribe(fileOutput);
  }

public:

  ~bulk_server() {
    stop();
  }

  static auto make(unsigned short port_num,
                  unsigned long long block_size,
                  unsigned long long max_cmds_in_files) {
    return std::shared_ptr<bulk_server>(new bulk_server(port_num,
                                                        block_size,
                                                        max_cmds_in_files));
  }

  void start() {
    if(isStarted)
      return;
    isStarted = true;
    auto session = client_session::make(io_service,
                                        sessions,
                                        cmdStorage,
                                        consoleOutput,
                                        fileOutput);
    do_accept(session);
    io_service.run();
  }

  void stop() {
    if(!isStarted)
      return;
    for(const auto& session : (*sessions)) {
      session->stop();
    }
    io_service.stop();
    isStarted = false;
  }

private:

  void do_accept(std::shared_ptr<client_session> session)
  {
    auto self = shared_from_this();
    acceptor.async_accept(session->sock(),
      [self, session](boost::system::error_code ec)
      {
        if (!ec) {
          self->sessions->insert(session);
          session->start();
        }
        else {
          BOOST_LOG_TRIVIAL(error) << ec.message();
          Logger::Instance().Flush();
        }
        auto new_session = client_session::make(self->io_service,
                                                self->sessions,
                                                self->cmdStorage,
                                                self->consoleOutput,
                                                self->fileOutput);
        self->do_accept(new_session);
      });
  }

  bool isStarted{false};
  boost::asio::io_service io_service;
  tcp::acceptor acceptor;
  session_set sessions;

  std::shared_ptr<Storage> cmdStorage;
  std::shared_ptr<ConsoleOutput> consoleOutput;
  std::shared_ptr<FileOutput> fileOutput;
};
