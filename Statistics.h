#pragma once

struct Statistics {
  size_t commands{0};
  size_t blocks{0};
};

std::ostream& operator<<(std::ostream& out, const Statistics& statistics) {
  out << statistics.blocks << " блок, "
      << statistics.commands << " команд";
  return out;
}

class BaseStatistics {

public:

  auto GetStatisctics() const {
    return statistics;
  }

protected:

  Statistics statistics;
};
