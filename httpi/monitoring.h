#pragma once

#include <memory>
#include <string>
#include <ctime>

#include "html/html.h"
#include "webjob.h"

class MonitoringJob : public WebJob {
    struct Stats {
        size_t utime = 0;
        size_t stime = 0;
        size_t vsize = 0;
    };

    Stats GetMonitoringStats() const;
    std::string TimeToStr(std::time_t* t) const;
  public:
    void Do() override;
    std::string name() const { return "Monitoring"; }
};
