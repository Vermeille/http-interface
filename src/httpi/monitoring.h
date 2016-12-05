#pragma once

#include <ctime>
#include <memory>
#include <string>

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
    std::atomic<bool> running_;
    const int refresh_delay_;
    const int history_size_;

   public:
    MonitoringJob(int refresh_delay, int history_size)
        : running_(true),
          refresh_delay_(refresh_delay),
          history_size_(history_size) {}

    void Stop() override { running_ = false; }

    void Do() override;
    std::string name() const override { return "Monitoring"; }
};
