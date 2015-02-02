#pragma once

#include <time.h>

class Clock {
  public:
    typedef long long int USec;
    Clock() {
      clock_gettime(CLOCK_MONOTONIC, &ts);
    }
    USec GetInUSec() const {
      return ((ts.tv_sec * 1000000) + (ts.tv_nsec / 1000));
    }
    USec GetDiff(const Clock &with) const {
      return (GetInUSec() - with.GetInUSec());
    }
    USec MeasureAndPrint(const std::string &test,
                         const std::string &stage,
                         unsigned           amount) const {
      if (amount < 1)
        amount = 1;
      const USec kResTm = Clock().GetDiff(*this) / amount;
      std::cout << "\t * " << test << ": " << stage << ": "
                << "[" << amount << "] "
                << kResTm << " usec"
                << std::endl;
      return kResTm;
    }
    USec MeasureAndPrint(const std::string &test,
                         const std::string &stage) const {
      return MeasureAndPrint(test, stage, 1);
    }
  private:
    struct timespec ts;
};
