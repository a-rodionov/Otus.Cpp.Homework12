#pragma once

#include <fstream>
#include <atomic>
#include <shared_mutex>
#include "IOutput.h"
#include "ThreadPool.h"
#include "Statistics.h"

std::string MakeFilename(std::size_t timestamp, unsigned short counter) {
  std::stringstream filename;
  filename << "bulk" << std::to_string(timestamp) << "_" << counter << ".log";
  return filename.str();
}

void WriteBulkToFile(const std::size_t timestamp, const std::list<std::string>& data, unsigned short counter) {
  auto filename = MakeFilename(timestamp, counter);
  std::ofstream ofs{filename.c_str(), std::ofstream::out | std::ofstream::trunc};
  if(ofs.fail())
    throw std::runtime_error("WriteBulkToFile. Can't open file for output.");
  OutputFormattedBulk(ofs, data);
  auto is_failed = ofs.fail();
  ofs.close();
  if(is_failed)
    throw std::runtime_error("WriteBulkToFile. Failed to write to file.");
}

class FileOutput : public IOutput, public ThreadPool
{
public:

  explicit FileOutput(size_t threads_count = 1)
  {
    for(decltype(threads_count) i{0}; i < threads_count; ++i) {
      AddWorker();
    }
  }

  void AddWorker() {
    // Новые потоки обработчики не добавляются до тех пор пока предшевствующая
    // команда StopWorkers не остановит текущие потоки обработчики и не получит
    // их статистику. Или пока выполняется поиск в threads_statistics одним из
    // потоков обработчиков.
    std::lock_guard<std::shared_timed_mutex> lock_statistics(statistics_mutex);
    threads_statistics[ThreadPool::AddWorker()] = Statistics{};
  }

  auto StopWorkers() {
    // Несмотря на то, что данная блокировка shared_lock и предполагает множественный доступ
    // на чтение, здесь она используется и для модификации разделяемых данных - threads_statistics.
    // С одной стороны shared_lock позволяет потокам обработчикам завершиться, поскольку в их коде
    // также используется shared_lock, а не unique_lock и их блокировки не последует.
    // С другой, блокирует функцию AddWorker, в которой выполняется модификация threads_statistics.
    std::shared_lock<std::shared_timed_mutex> lock_statistics(statistics_mutex);
    ThreadPool::StopWorkers();
    // Код ниже будет исполняться, когда потоки обработчики уже будут завершены, поэтому 
    // модификация threads_statistics безопасна.
    auto threads_statistics_copy = threads_statistics;
    threads_statistics.clear();
    return threads_statistics_copy;
  }

  void Output(const std::size_t timestamp, std::shared_ptr<const std::list<std::string>>& data) override {
    auto unique_counter = counter++;
    AddTask([this, timestamp, data, unique_counter]() {
      WriteBulkToFile(timestamp, *data, unique_counter);

      std::shared_lock<std::shared_timed_mutex> lock_statistics(statistics_mutex);
      auto statistics = threads_statistics.find(std::this_thread::get_id());
      if(std::cend(threads_statistics) == statistics)
        throw std::runtime_error("FileOutput::Output. Statistics for this thread doesn't exist.");
      lock_statistics.unlock();
      // Каждый поток модифицирует только свою статистику, поэтому безопасно ее модифицировать без блокировки.
      ++statistics->second.blocks;
      statistics->second.commands += data->size();

      std::lock_guard<std::shared_timed_mutex> lock_filenames(filenames_mutex);
      processed_filenames.push_back(MakeFilename(timestamp, unique_counter));
    });
  }

  auto GetProcessedFilenames() const {
    std::shared_lock<std::shared_timed_mutex> lock_filenames(filenames_mutex);
    return processed_filenames;
  }

protected:

  std::map<std::thread::id, Statistics> threads_statistics;
  std::shared_timed_mutex statistics_mutex;

  std::vector<std::string> processed_filenames;
  unsigned short counter{0};
  mutable std::shared_timed_mutex filenames_mutex;
};
