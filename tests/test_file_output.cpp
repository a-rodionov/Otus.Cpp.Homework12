#include <sys/file.h>
#include <sstream>
#include "Storage.h"
#include "FileOutput.h"
#include "CommandProcessor.h"

#define BOOST_TEST_MODULE test_file_output

#include <boost/test/unit_test.hpp>
#include <boost/test/included/unit_test.hpp>


BOOST_AUTO_TEST_SUITE(test_suite_main)

BOOST_AUTO_TEST_CASE(simple_file_output)
{
  std::string result;
  std::string goodResult{"bulk: cmd100, cmd200, cmd300"};
  std::string testData{"cmd100\ncmd200\ncmd300\n"};

  auto commandProcessor = std::make_unique<CommandProcessor>();
  auto storage = std::make_shared<Storage>(3);
  auto fileOutput = std::make_shared<FileOutput>(1, true);

  storage->Subscribe(fileOutput);
  commandProcessor->Subscribe(storage);

  commandProcessor->Process(&testData.front(), testData.size(), true);

  auto file_statistics = fileOutput->StopWorkers();

  BOOST_REQUIRE_EQUAL(1, file_statistics.size());
  auto filenames = fileOutput->GetProcessedFilenames();
  BOOST_REQUIRE_EQUAL(1, filenames.size());

  std::ifstream ifs{filenames.at(0).c_str(), std::ifstream::in};
  BOOST_CHECK_EQUAL(false, ifs.fail());

  std::getline(ifs, result);
  BOOST_CHECK_EQUAL(goodResult, result);

  std::getline(ifs, result);
  BOOST_CHECK_EQUAL(true, ifs.eof());
  goodResult.clear();
  BOOST_CHECK_EQUAL(goodResult, result);

  std::remove(filenames.at(0).c_str());
}

BOOST_AUTO_TEST_CASE(file_output_to_locked_file)
{
  std::string result;
  std::string goodResult{"bulk: cmd1, cmd2, cmd3"};
  std::list<std::string> testData{"cmd1", "cmd2", "cmd3"};
  size_t timestamp {123};
  unsigned short counter {0};
  auto filename = MakeFilename(timestamp, counter);

  std::remove(filename.c_str());

  auto file_handler = open(filename.c_str(), O_CREAT);
  BOOST_REQUIRE_EQUAL(true, -1 != file_handler);
  BOOST_REQUIRE_EQUAL(true, -1 != flock( file_handler, LOCK_EX | LOCK_NB ));
  
  BOOST_CHECK_THROW(WriteBulkToFile(timestamp, testData, counter), std::runtime_error);
  
  flock(file_handler, LOCK_UN | LOCK_NB);
  close(file_handler);
  std::remove(filename.c_str());
}

BOOST_AUTO_TEST_SUITE_END()
