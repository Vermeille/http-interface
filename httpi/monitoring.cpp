#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include "job.h"

DEFINE_int32(status_memory, 20, "the number of samples to show in charts");
DEFINE_int32(status_refresh, 30, "the refresh frequency (in secs) for monitoring info");

struct Stats {
    size_t utime = 0;
    size_t stime = 0;
    size_t vsize = 0;
};

Stats GetMonitoringStats() {
    static const int kUserTimeIndex = 14;
    static const int kKernelTimeIndex = 15;
    static const int kVirtualSizeIndex = 23;

    std::ifstream stat("/proc/self/stat");
    Stats s;

    int i = 1;
    while (stat) {
        if (i == kUserTimeIndex)
            stat >> s.utime;
        else if (i == kKernelTimeIndex)
            stat >> s.stime;
        else if (i == kVirtualSizeIndex)
            stat >> s.vsize;
        else if (i == 2 || i == 3) {
            std::string ignore;
            stat >> ignore;
        } else {
            size_t ignore;
            stat >> ignore;
        }

        ++i;
    }
    return s;
}

std::string TimeToStr(std::time_t* t) {
    std::string str(std::ctime(t));
    str.pop_back();
    return str;
}

void MonitoringJob(const std::vector<std::string>& vs, JobStatus& job) {
    Chart ram("RAM_usage");
    Chart cpu("CPU_usage");

    ram.Label("time").Value("ram");
    cpu.Label("time").Value("cpu");

    size_t last_time = 0;
    while (ServiceRuns()) {
        auto begin = std::chrono::system_clock::now();

        Stats s = GetMonitoringStats();

        ram.Log("ram", s.vsize);
        cpu.Log("cpu", (100 / FLAGS_status_refresh)
                * ((double)s.utime +  s.stime - last_time) / sysconf(_SC_CLK_TCK));

        auto t = std::chrono::system_clock::to_time_t(begin);
        ram.Log("time", TimeToStr(&t));
        cpu.Log("time", TimeToStr(&t));
        ram.MostRecent(FLAGS_status_memory);
        cpu.MostRecent(FLAGS_status_memory);

        last_time = s.utime + s.stime;

        job.SetPage(Html() << ram.Get() << cpu.Get());

        std::this_thread::sleep_for(std::chrono::seconds(FLAGS_status_refresh)
                - (std::chrono::system_clock::now() - begin));
    }
}

