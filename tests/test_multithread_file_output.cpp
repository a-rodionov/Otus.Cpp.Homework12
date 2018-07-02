#include <sstream>
#include "Storage.h"
#include "FileOutput.h"
#include "CommandProcessor.h"

#define BOOST_TEST_MODULE test_multithread_file_output

#include <boost/test/unit_test.hpp>
#include <boost/test/included/unit_test.hpp>


BOOST_AUTO_TEST_SUITE(test_suite_main)

BOOST_AUTO_TEST_CASE(verify_statistics)
{
  std::string testData{"cmd1\ncmd2\ncmd3\n"
                      "{\ncmd4\n\ncmd6\ncmd7\n}\n"
                      "cmd9\ncmd8\ncmd10\n"
                      "cmd11\ncmd12\ncmd13\n"
                      "cmd14\n"};

  auto commandProcessor = std::make_unique<CommandProcessor>();
  auto storage = std::make_shared<Storage>(3);
  auto fileOutput = std::make_shared<FileOutput>(2, true);

  storage->Subscribe(fileOutput);
  commandProcessor->Subscribe(storage);

  commandProcessor->Process(&testData.front(), testData.size(), true);

  auto file_statistics = fileOutput->StopWorkers();

  auto main_statisctics = storage->GetStatisctics();
  decltype(main_statisctics) threads_statisctics;
  for(const auto& statistic : file_statistics) {
    threads_statisctics.commands += statistic.second.commands;
    threads_statisctics.blocks += statistic.second.blocks;
  }

  auto filenames = fileOutput->GetProcessedFilenames();
  for(const auto& filename : filenames) {
    std::remove(filename.c_str());
  }

  BOOST_REQUIRE_EQUAL(main_statisctics.commands, threads_statisctics.commands);
  BOOST_REQUIRE_EQUAL(main_statisctics.blocks, threads_statisctics.blocks);

  BOOST_REQUIRE(main_statisctics.commands != std::cbegin(file_statistics)->second.commands);
  BOOST_REQUIRE(main_statisctics.blocks != std::cbegin(file_statistics)->second.blocks);
}

BOOST_AUTO_TEST_CASE(verify_unique_filenames)
{
  std::string testData{"cmd1\ncmd2\ncmd3\n"
                      "{\ncmd4\n\ncmd6\ncmd7\n}\n"
                      "cmd9\ncmd8\ncmd10\n"
                      "cmd11\ncmd12\ncmd13\n"
                      "cmd14\ncmd15\ncmd16\n"
                      "cmd17\ncmd18\ncmd19\n"
                      "cmd20\ncmd21\ncmd22\n"
                      "cmd23\n"};

  auto commandProcessor = std::make_unique<CommandProcessor>();
  auto storage = std::make_shared<Storage>(3);
  auto fileOutput = std::make_shared<FileOutput>(3, true);

  storage->Subscribe(fileOutput);
  commandProcessor->Subscribe(storage);

  commandProcessor->Process(&testData.front(), testData.size(), true);

  auto file_statistics = fileOutput->StopWorkers();
  auto filenames = fileOutput->GetProcessedFilenames();
  for(const auto& filename : filenames) {
    std::remove(filename.c_str());
  }

  std::sort(std::begin(filenames), std::end(filenames));
  BOOST_REQUIRE(std::cend(filenames) == std::adjacent_find(std::cbegin(filenames), std::cend(filenames)));
}

BOOST_AUTO_TEST_SUITE_END()
