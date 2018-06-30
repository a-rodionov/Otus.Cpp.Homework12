#pragma once

#include <iostream>
#include <shared_mutex>
#include "IOutput.h"
#include "ThreadPool.h"
#include "Statistics.h"

class ConsoleOutput : public IOutput, public ThreadPool
{

public:

  explicit ConsoleOutput(std::ostream& out, size_t threads_count = 1)
    : out{out}
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

  void Output(const std::size_t, std::shared_ptr<const std::list<std::string>>& data) override {
    AddTask([this, data]() {

      OutputFormattedBulk(out, *data);

      // shared_lock используется потому что состав коллекции threads_statistics изменен не будет.
      // В threads_statistics необходимо лишь выполнить поиск структуры соответствующей
      // данному потоку.
      std::shared_lock<std::shared_timed_mutex> lock_statistics(statistics_mutex);
      auto statistics = threads_statistics.find(std::this_thread::get_id());
      if(std::cend(threads_statistics) == statistics)
        throw std::runtime_error("ConsoleOutput::Output. Statistics for this thread doesn't exist.");
      lock_statistics.unlock();
      // Каждый поток модифицирует только свою статистику, поэтому безопасно ее модифицировать без блокировки.
      ++statistics->second.blocks;
      statistics->second.commands += data->size();
    });
  }

private:

  std::ostream& out;

  std::map<std::thread::id, Statistics> threads_statistics;
  std::shared_timed_mutex statistics_mutex;
};
